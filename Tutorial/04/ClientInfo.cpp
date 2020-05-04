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

	//I/O Completion Port��ü�� ������ �����Ų��.
	if (false == BindIOCompletionPort(hIOCP))
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
	shutdown(mSock, SD_BOTH);

	//���� �ɼ��� �����Ѵ�.
	setsockopt(mSock, SOL_SOCKET, SO_LINGER, (char*)&stLinger, sizeof(stLinger));

	//���� ������ ���� ��Ų��. 
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
	//socket�� pClientInfo�� CompletionPort��ü�� �����Ų��.
	HANDLE hIOCP = CreateIoCompletionPort((HANDLE)GetSock()
		, hIOCP_
		, (ULONG_PTR)(this), 0);

	if (hIOCP == INVALID_HANDLE_VALUE)
	{
		std::cerr << "[����] CreateIoCompletionPort()�Լ� ����: " << GetLastError() << std::endl;
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
	if (nRet == SOCKET_ERROR && (::WSAGetLastError() != ERROR_IO_PENDING))
	{
		std::cerr << "[����] WSARecv()�Լ� ���� : " << ::WSAGetLastError() << std::endl;
		return false;
	}

	return true;
}

bool
stClientInfo::SendMsg(UINT32 dataSize, const char* pMsg) const
{
	// ������ ���߿� �����ʿ��� ���ش�. ...�̰� ���� ����� �ƴ� �� ������;
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

	//socket_error�̸� client socket�� �������ɷ� ó���Ѵ�.
	if (nRet == SOCKET_ERROR && (::WSAGetLastError() != ERROR_IO_PENDING))
	{
		std::cerr << "[����] WSASend()�Լ� ���� : " << ::WSAGetLastError() << std::endl;
		return false;
	}

	return true;
}

void
stClientInfo::SendCompleted(UINT32 dataSize)
{
	std::cout << "[�۽� �Ϸ�] bytes : " << dataSize << std::endl;
}