#include <iostream>
#include "ClientInfo.h"

stClientInfo::stClientInfo(UINT index)
	: mIndex(index), mSock(INVALID_SOCKET)
{
	ZeroMemory(&mRecvOverlappedEx, sizeof(stOverlappedEx));
}

bool
stClientInfo::OnConnect(HANDLE hIOCP, SOCKET socket)
{
	mSock = socket;

	Clear();

	//I/O Completion Port객체와 소켓을 연결시킨다.
	if (false == BindIOCompletionPort(hIOCP))
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
	shutdown(mSock, SD_BOTH);

	//소켓 옵션을 설정한다.
	setsockopt(mSock, SOL_SOCKET, SO_LINGER, (char*)&stLinger, sizeof(stLinger));

	//소켓 연결을 종료 시킨다. 
	closesocket(mSock);
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
	HANDLE hIOCP = CreateIoCompletionPort((HANDLE)GetSock()
		, hIOCP_
		, (ULONG_PTR)(this), 0);

	if (hIOCP == INVALID_HANDLE_VALUE)
	{
		std::cerr << "[에러] CreateIoCompletionPort()함수 실패: " << GetLastError() << std::endl;
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
	if (nRet == SOCKET_ERROR && (::WSAGetLastError() != ERROR_IO_PENDING))
	{
		std::cerr << "[에러] WSARecv()함수 실패 : " << ::WSAGetLastError() << std::endl;
		return false;
	}

	return true;
}

bool
stClientInfo::SendMsg(UINT32 dataSize, const char* pMsg) const
{
	// 해제는 나중에 서버쪽에서 해준다. ...이게 좋은 방법은 아닌 것 같은데;
	stOverlappedEx* pOverlappedEx = new stOverlappedEx();
	ZeroMemory(pOverlappedEx, sizeof(stOverlappedEx));
	pOverlappedEx->m_wsaBuf.len = dataSize;
	pOverlappedEx->m_wsaBuf.buf = new char[dataSize];
	CopyMemory(pOverlappedEx->m_wsaBuf.buf, pMsg, dataSize); 
	pOverlappedEx->m_eOperation = IOOperation::SEND;

	DWORD dwRecvNumBytes = 0;
	int nRet = ::WSASend(mSock,
		&(pOverlappedEx->m_wsaBuf),
		1,
		&dwRecvNumBytes,
		0,
		(LPWSAOVERLAPPED)pOverlappedEx,
		NULL);

	//socket_error이면 client socket이 끊어진걸로 처리한다.
	if (nRet == SOCKET_ERROR && (::WSAGetLastError() != ERROR_IO_PENDING))
	{
		std::cerr << "[에러] WSASend()함수 실패 : " << ::WSAGetLastError() << std::endl;
		return false;
	}

	return true;
}

void
stClientInfo::SendCompleted(UINT32 dataSize)
{
	std::cout << "[송신 완료] bytes : " << dataSize << std::endl;
}