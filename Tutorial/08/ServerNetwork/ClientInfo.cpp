#include <iostream>
#include <chrono>
#include <ws2tcpip.h>
#include <mswsock.h>
#include "ClientInfo.h"

stClientInfo::stClientInfo(int index, HANDLE hIOCP)
	: mIndex(index), mIOCPHandle(hIOCP)
{
	ZeroMemory(&mRecvOverlappedEx, sizeof(stOverlappedEx));
}

bool
stClientInfo::OnConnect(HANDLE hIOCP, SOCKET socket)
{
	mSocket = socket;
	mIsConnect = true;

	Clear();

	//I/O Completion Port객체와 소켓을 연결시킨다.
	if (BindIOCompletionPort(hIOCP) == false)
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
	if (bIsForce)
	{
		stLinger.l_onoff = 1;
	}

	//socketClose소켓의 데이터 송수신을 모두 중단 시킨다.
	::shutdown(mSocket, SD_BOTH);

	//소켓 옵션을 설정한다.
	::setsockopt(mSocket, SOL_SOCKET, SO_LINGER, (char*)&stLinger, sizeof(stLinger));

	mIsConnect = false;
	mLatestClosedTimeSec = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
	
	//소켓 연결을 종료 시킨다.
	::closesocket(mSocket);
	mSocket = INVALID_SOCKET;
}

void
stClientInfo::Clear()
{

}

bool
stClientInfo::PostAccept(SOCKET listenSock, uint64_t curTimeSec)
{
	std::cout << "PostAccept. client Index: " << GetIndex() << std::endl;

	mLatestClosedTimeSec = UINT32_MAX;

	mSocket = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == mSocket)
	{
		std::cerr << "client Socket WSASocket Error : " << ::GetLastError() << std::endl;
		return false;
	}

	ZeroMemory(&mAcceptContext, sizeof(stOverlappedEx));

	DWORD bytes = 0;
	DWORD flags = 0;
	mAcceptContext.m_wsaBuf.len = 0;
	mAcceptContext.m_wsaBuf.buf = nullptr;
	mAcceptContext.m_eOperation = IOOperation::ACCEPT;
	mAcceptContext.SessionIndex = mIndex;

	if (FALSE == AcceptEx(listenSock, mSocket, mAcceptBuf, 0,
		sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &bytes, (LPWSAOVERLAPPED) & (mAcceptContext)))
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			std::cout << "AcceptEx Error : " << ::GetLastError() << std::endl;
			return false;
		}
	}

	return true;
}

bool
stClientInfo::AcceptCompletion()
{
	printf_s("AcceptCompletion : SessionIndex(%d)\n", mIndex);

	if (!OnConnect(mIOCPHandle, mSocket))
	{
		return false;
	}

	SOCKADDR_IN		stClientAddr;
	int nAddrLen = sizeof(SOCKADDR_IN);
	char clientIP[32] = { 0, };
	inet_ntop(AF_INET, &(stClientAddr.sin_addr), clientIP, 32 - 1);
	printf(u8"클라이언트 접속 : IP(%s) SOCKET(%d)\n", clientIP, (int)mSocket);

	return true;
}

bool
stClientInfo::BindIOCompletionPort(HANDLE hIOCP_)
{
	//socket과 pClientInfo를 CompletionPort객체와 연결시킨다.
	auto hIOCP = CreateIoCompletionPort((HANDLE)GetSock(), hIOCP_, (ULONG_PTR)this, 0);
	if (hIOCP == INVALID_HANDLE_VALUE)
	{
		std::cout << u8"[에러] CreateIoCompletionPort()함수 실패: " << ::GetLastError() << std::endl;
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
	mRecvOverlappedEx.m_wsaBuf.len = MAX_SOCK_RECVBUF;
	mRecvOverlappedEx.m_wsaBuf.buf = mRecvBuf;
	mRecvOverlappedEx.m_eOperation = IOOperation::RECV;

	int nRet = ::WSARecv(mSocket, &(mRecvOverlappedEx.m_wsaBuf), 1, &dwRecvNumBytes, &dwFlag, (LPWSAOVERLAPPED) & (mRecvOverlappedEx), NULL);

	//socket_error이면 client socket이 끊어진걸로 처리한다.
	if (nRet == SOCKET_ERROR && (::WSAGetLastError() != ERROR_IO_PENDING))
	{
		std::cerr << u8"[에러] WSARecv()함수 실패 : " << ::WSAGetLastError() << std::endl;
		return false;
	}

	return true;
}

bool
stClientInfo::SendMsg(UINT32 dataSize, const char* pMsg)
{
	auto sendOverlappedEx = new stOverlappedEx;
	ZeroMemory(sendOverlappedEx, sizeof(stOverlappedEx));
	sendOverlappedEx->m_wsaBuf.len = dataSize;
	sendOverlappedEx->m_wsaBuf.buf = new char[dataSize];
	CopyMemory(sendOverlappedEx->m_wsaBuf.buf, pMsg, dataSize);
	sendOverlappedEx->m_eOperation = IOOperation::SEND;

	std::lock_guard<std::mutex> guard(mSendLock);

	mSendDataqueue.push(sendOverlappedEx);

	if (mSendDataqueue.size() == 1)
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

	delete[] mSendDataqueue.front()->m_wsaBuf.buf;
	delete mSendDataqueue.front();

	mSendDataqueue.pop();

	if (mSendDataqueue.empty() == false)
	{
		SendIO();
	}
}

bool
stClientInfo::SendIO()
{
	auto sendOverlappedEx = mSendDataqueue.front();

	DWORD dwRecvNumBytes = 0;
	int nRet = WSASend(mSocket,
		&(sendOverlappedEx->m_wsaBuf),
		1,
		&dwRecvNumBytes,
		0,
		(LPWSAOVERLAPPED)sendOverlappedEx,
		NULL);

	//socket_error이면 client socket이 끊어진걸로 처리한다.
	if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
	{
		printf("[에러] WSASend()함수 실패 : %d\n", WSAGetLastError());
		return false;
	}

	return true;
}

bool
stClientInfo::SetSocketOption()
{
	/*if (SOCKET_ERROR == setsockopt(mSock, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)GIocpManager->GetListenSocket(), sizeof(SOCKET)))
		{
			printf_s("[DEBUG] SO_UPDATE_ACCEPT_CONTEXT error: %d\n", GetLastError());
			return false;
		}*/

	int opt = 1;
	if (SOCKET_ERROR == setsockopt(mSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)&opt, sizeof(int)))
	{
		printf_s("[DEBUG] TCP_NODELAY error: %d\n", GetLastError());
		return false;
	}

	opt = 0;
	if (SOCKET_ERROR == setsockopt(mSocket, SOL_SOCKET, SO_RCVBUF, (const char*)&opt, sizeof(int)))
	{
		printf_s("[DEBUG] SO_RCVBUF change error: %d\n", GetLastError());
		return false;
	}

	return true;
}
