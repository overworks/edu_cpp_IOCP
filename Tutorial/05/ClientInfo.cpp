#include <iostream>
#include "ClientInfo.h"

stClientInfo::stClientInfo(UINT sessionIndex)
	: mIndex(sessionIndex), mSock(INVALID_SOCKET)
{
	ZeroMemory(&mRecvOverlappedEx, sizeof(stOverlappedEx));
	ZeroMemory(&mSendOverlappedEx, sizeof(stOverlappedEx));
}

bool
stClientInfo::OnConnect(HANDLE hIOCP, SOCKET socket)
{
	mSock = socket;

	Clear();

	//I/O Completion Port객체와 소켓을 연결시킨다.
	if (!BindIOCompletionPort(hIOCP))
	{
		return false;
	}

	return BindRecv();
}

void
stClientInfo::Close(bool bIsForce)
{
	struct linger stLinger = { 0, 0 };	// SO_DONTLINGER로 설정

	// bIsForce가 true이면 SO_LINGER, timeout = 0으로 설정하여 강제 종료 시킨다. 주의 : 데이터 손실이 있을수 있음 
	if (true == bIsForce)
	{
		stLinger.l_onoff = 1;
	}

	//socketClose소켓의 데이터 송수신을 모두 중단 시킨다.
	::shutdown(mSock, SD_BOTH);

	//소켓 옵션을 설정한다.
	::setsockopt(mSock, SOL_SOCKET, SO_LINGER, (char*)&stLinger, sizeof(stLinger));

	//소켓 연결을 종료 시킨다. 
	::closesocket(mSock);
	mSock = INVALID_SOCKET;
}

void
stClientInfo::Clear()
{
	mSendPos = 0;
	mIsSending = false;
}

bool
stClientInfo::BindIOCompletionPort(HANDLE hIOCP_)
{
	//socket과 pClientInfo를 CompletionPort객체와 연결시킨다.
	HANDLE hIOCP = CreateIoCompletionPort((HANDLE)GetSock()
		, hIOCP_
		, (ULONG_PTR)(this), 0);

	if (hIOCP == INVALID_HANDLE_VALUE)
	{
		std::cerr << "[에러] CreateIoCompletionPort()함수 실패: " << ::GetLastError() << std::endl;
		return false;
	}

	return true;
}

bool
stClientInfo::BindRecv()
{
	DWORD dwFlag = 0;
	DWORD dwRecvNumBytes = 0;

	//Overlapped I/O을 위해 각 정보를 셋팅해 준다.
	mRecvOverlappedEx.m_wsaBuf.len = MAX_SOCKBUF;
	mRecvOverlappedEx.m_wsaBuf.buf = mRecvBuf;
	mRecvOverlappedEx.m_eOperation = IOOperation::RECV;

	int nRet = WSARecv(mSock,
		&(mRecvOverlappedEx.m_wsaBuf),
		1,
		&dwRecvNumBytes,
		&dwFlag,
		(LPWSAOVERLAPPED) & (mRecvOverlappedEx),
		NULL);

	//socket_error이면 client socket이 끊어진걸로 처리한다.
	if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
	{
		std::cerr << "[에러] WSARecv()함수 실패 : " << ::WSAGetLastError() << std::endl;
		return false;
	}

	return true;
}

bool
stClientInfo::SendMsg(UINT32 dataSize, const char* pMsg)
{
	std::lock_guard<std::mutex> guard(mSendLock);

	if ((mSendPos + dataSize) > MAX_SOCK_SENDBUF)
	{
		mSendPos = 0;
	}

	char* pSendBuf = mSendBuf + mSendPos;

	//전송될 메세지를 복사
	CopyMemory(pSendBuf, pMsg, dataSize);
	mSendPos += dataSize;

	return true;
}

bool
stClientInfo::SendIO()
{
	if (mSendPos <= 0 || mIsSending)
	{
		return true;
	}

	std::lock_guard<std::mutex> guard(mSendLock);

	mIsSending = true;

	CopyMemory(mSendingBuf, mSendBuf, mSendPos);

	//Overlapped I/O을 위해 각 정보를 셋팅해 준다.
	mSendOverlappedEx.m_wsaBuf.len = mSendPos;
	mSendOverlappedEx.m_wsaBuf.buf = mSendingBuf;
	mSendOverlappedEx.m_eOperation = IOOperation::SEND;

	DWORD dwRecvNumBytes = 0;
	int nRet = WSASend(mSock,
		&(mSendOverlappedEx.m_wsaBuf),
		1,
		&dwRecvNumBytes,
		0,
		(LPWSAOVERLAPPED) & (mSendOverlappedEx),
		NULL);

	//socket_error이면 client socket이 끊어진걸로 처리한다.
	if (nRet == SOCKET_ERROR && (::WSAGetLastError() != ERROR_IO_PENDING))
	{
		std::cerr << "[에러] WSASend()함수 실패 : " << ::WSAGetLastError() << std::endl;
		return false;
	}

	mSendPos = 0;
	return true;
}

void
stClientInfo::SendCompleted(UINT32 dataSize)
{
	mIsSending = false;
	std::cout << "[송신 완료] bytes : " << dataSize << std::endl;
}