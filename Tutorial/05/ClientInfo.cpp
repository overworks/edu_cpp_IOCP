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

	//I/O Completion Port��ü�� ������ �����Ų��.
	if (!BindIOCompletionPort(hIOCP))
	{
		return false;
	}

	return BindRecv();
}

void
stClientInfo::Close(bool bIsForce)
{
	struct linger stLinger = { 0, 0 };	// SO_DONTLINGER�� ����

	// bIsForce�� true�̸� SO_LINGER, timeout = 0���� �����Ͽ� ���� ���� ��Ų��. ���� : ������ �ս��� ������ ���� 
	if (true == bIsForce)
	{
		stLinger.l_onoff = 1;
	}

	//socketClose������ ������ �ۼ����� ��� �ߴ� ��Ų��.
	::shutdown(mSock, SD_BOTH);

	//���� �ɼ��� �����Ѵ�.
	::setsockopt(mSock, SOL_SOCKET, SO_LINGER, (char*)&stLinger, sizeof(stLinger));

	//���� ������ ���� ��Ų��. 
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
	//socket�� pClientInfo�� CompletionPort��ü�� �����Ų��.
	HANDLE hIOCP = CreateIoCompletionPort((HANDLE)GetSock()
		, hIOCP_
		, (ULONG_PTR)(this), 0);

	if (hIOCP == INVALID_HANDLE_VALUE)
	{
		std::cerr << "[����] CreateIoCompletionPort()�Լ� ����: " << ::GetLastError() << std::endl;
		return false;
	}

	return true;
}

bool
stClientInfo::BindRecv()
{
	DWORD dwFlag = 0;
	DWORD dwRecvNumBytes = 0;

	//Overlapped I/O�� ���� �� ������ ������ �ش�.
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

	//socket_error�̸� client socket�� �������ɷ� ó���Ѵ�.
	if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
	{
		std::cerr << "[����] WSARecv()�Լ� ���� : " << ::WSAGetLastError() << std::endl;
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

	//���۵� �޼����� ����
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

	//Overlapped I/O�� ���� �� ������ ������ �ش�.
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

	//socket_error�̸� client socket�� �������ɷ� ó���Ѵ�.
	if (nRet == SOCKET_ERROR && (::WSAGetLastError() != ERROR_IO_PENDING))
	{
		std::cerr << "[����] WSASend()�Լ� ���� : " << ::WSAGetLastError() << std::endl;
		return false;
	}

	mSendPos = 0;
	return true;
}

void
stClientInfo::SendCompleted(UINT32 dataSize)
{
	mIsSending = false;
	std::cout << "[�۽� �Ϸ�] bytes : " << dataSize << std::endl;
}