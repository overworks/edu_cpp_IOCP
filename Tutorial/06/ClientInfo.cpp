#include <iostream>
#include "ClientInfo.h"

stClientInfo::stClientInfo(UINT index)
	: mIndex(index)
{
	ZeroMemory(&mRecvOverlappedEx, sizeof(stOverlappedEx));
	mSock = INVALID_SOCKET;
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

}

bool
stClientInfo::BindIOCompletionPort(HANDLE hIOCP_)
{
	//socket과 pClientInfo를 CompletionPort객체와 연결시킨다.
	HANDLE hIOCP = ::CreateIoCompletionPort((HANDLE)GetSock()
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

	int nRet = ::WSARecv(mSock,
		&(mRecvOverlappedEx.m_wsaBuf),
		1,
		&dwRecvNumBytes,
		&dwFlag,
		(LPWSAOVERLAPPED) & (mRecvOverlappedEx),
		NULL);

	//socket_error이면 client socket이 끊어진걸로 처리한다.
	if (nRet == SOCKET_ERROR && (::WSAGetLastError() != ERROR_IO_PENDING))
	{
		std::cerr << "[에러] WSARecv()함수 실패 : " << ::WSAGetLastError() << std::endl;
		return false;
	}

	return true;
}

bool
stClientInfo::SendMsg(UINT32 dataSize, const char* pMsg)
{
	stOverlappedEx* sendOverlappedEx = new stOverlappedEx;
	ZeroMemory(sendOverlappedEx, sizeof(stOverlappedEx));
	sendOverlappedEx->m_wsaBuf.len = dataSize;
	sendOverlappedEx->m_wsaBuf.buf = new char[dataSize];
	CopyMemory(sendOverlappedEx->m_wsaBuf.buf, pMsg, dataSize);
	sendOverlappedEx->m_eOperation = IOOperation::SEND;

	std::lock_guard<std::mutex> guard(mSendLock);

	mSendDataQueue.push(sendOverlappedEx);

	if (mSendDataQueue.size() == 1)
	{
		SendIO();
	}

	return true;
}

void
stClientInfo::SendCompleted(UINT32 dataSize)
{
	std::cout << "[송신 완료] bytes : " << dataSize << std::endl;

	std::lock_guard<std::mutex> guard(mSendLock);

	auto sendData = mSendDataQueue.front();
	delete[] sendData->m_wsaBuf.buf;
	delete sendData;

	mSendDataQueue.pop();

	if (!mSendDataQueue.empty())
	{
		SendIO();
	}
}

bool
stClientInfo::SendIO()
{
	auto sendOverlappedEx = mSendDataQueue.front();

	DWORD dwRecvNumBytes = 0;
	int nRet = ::WSASend(mSock,
		&(sendOverlappedEx->m_wsaBuf),
		1,
		&dwRecvNumBytes,
		0,
		(LPWSAOVERLAPPED)sendOverlappedEx,
		NULL);

	//socket_error이면 client socket이 끊어진걸로 처리한다.
	if (nRet == SOCKET_ERROR && (::WSAGetLastError() != ERROR_IO_PENDING))
	{
		std::cerr << "[에러] WSASend()함수 실패 : " << ::WSAGetLastError() << std::endl;
		return false;
	}

	return true;
}