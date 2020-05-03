#pragma comment(lib, "ws2_32")

#include <iostream>
#include <algorithm>
#include "IOCPServer.h"

IOCPServer::IOCPServer()
{

}

IOCPServer::~IOCPServer()
{
	//������ ����� ������.
	::WSACleanup();
}

bool
IOCPServer::InitSocket()
{
	::WSADATA wsaData;

	int nRet = ::WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (0 != nRet)
	{
		std::cerr << "[����] WSAStartup()�Լ� ���� : " << ::WSAGetLastError() << std::endl;
		return false;
	}

	//���������� TCP , Overlapped I/O ������ ����
	mListenSocket = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == mListenSocket)
	{
		std::cerr << "[����] socket()�Լ� ���� : " << ::WSAGetLastError() << std::endl;
		return false;
	}

	std::cout << "���� �ʱ�ȭ ����" << std::endl;
	return true;
}

bool
IOCPServer::BindAndListen(int nBindPort)
{
	::SOCKADDR_IN	stServerAddr;
	stServerAddr.sin_family = AF_INET;
	stServerAddr.sin_port = htons(nBindPort); //���� ��Ʈ�� �����Ѵ�.		
	//� �ּҿ��� ������ �����̶� �޾Ƶ��̰ڴ�.
	//���� ������� �̷��� �����Ѵ�. ���� �� �����ǿ����� ������ �ް� �ʹٸ�
	//�� �ּҸ� inet_addr�Լ��� �̿��� ������ �ȴ�.
	stServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	//������ ������ ���� �ּ� ������ cIOCompletionPort ������ �����Ѵ�.
	int nRet = ::bind(mListenSocket, (SOCKADDR*)&stServerAddr, sizeof(SOCKADDR_IN));
	if (0 != nRet)
	{
		std::cerr << "[����] bind()�Լ� ���� : " << ::WSAGetLastError() << std::endl;
		return false;
	}

	//���� ��û�� �޾Ƶ��̱� ���� cIOCompletionPort������ ����ϰ� 
	//���Ӵ��ť�� 5���� ���� �Ѵ�.
	nRet = ::listen(mListenSocket, 5);
	if (0 != nRet)
	{
		std::cerr << "[����] listen()�Լ� ���� : " << ::WSAGetLastError() << std::endl;
		return false;
	}

	std::cout << "���� ��� ���� - Port: " << nBindPort << std::endl;
	return true;
}

bool
IOCPServer::StartServer(UINT32 maxClientCount)
{
	CreateClient(maxClientCount);

	//CompletionPort��ü ���� ��û�� �Ѵ�.
	mIOCPHandle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, MAX_WORKERTHREAD);
	if (NULL == mIOCPHandle)
	{
		std::cerr << "[����] CreateIoCompletionPort()�Լ� ����: " << ::GetLastError() << std::endl;
		return false;
	}

	//���ӵ� Ŭ���̾�Ʈ �ּ� ������ ������ ����ü
	bool bRet = CreateWorkerThread();
	if (false == bRet) {
		std::cerr << "[����] WorkerThread ���� ����" << std::endl;
		return false;
	}

	bRet = CreateAccepterThread();
	if (false == bRet) {
		std::cerr << "[����] AccepterThread ���� ����" << std::endl;
		return false;
	}

	std::cout << "���� ����" << std::endl;
	return true;
}

void
IOCPServer::DestroyThread()
{
	auto joinThread = [](std::thread& th) { if (th.joinable()) th.join(); };

	// Worker ������ ����
	mIsWorkerRun = false;
	CloseHandle(mIOCPHandle);
	std::for_each(mIOWorkerThreads.begin(), mIOWorkerThreads.end(), joinThread);
	
	// Accepter �����带 ����
	mIsAccepterRun = false;
	closesocket(mListenSocket);
	joinThread(mAccepterThread);
}

void
IOCPServer::CreateClient(UINT32 maxClientCount)
{
	for (UINT32 i = 0; i < maxClientCount; ++i)
	{
		mClientInfos.emplace_back(i);
	}
}

bool
IOCPServer::CreateWorkerThread()
{
	//WaingThread Queue�� ��� ���·� ���� ������� ���� ����Ǵ� ���� : (cpu���� * 2) + 1 
	for (int i = 0; i < MAX_WORKERTHREAD; i++)
	{
		mIOWorkerThreads.emplace_back([this]() { WokerThread(); });
	}

	std::cout << "WokerThread ����.." << std::endl;
	return true;
}

bool
IOCPServer::CreateAccepterThread()
{
	mAccepterThread = std::thread([this]() { AccepterThread(); });

	std::cout << "AccepterThread ����.." << std::endl;
	return true;
}

stClientInfo*
IOCPServer::GetEmptyClientInfo()
{
	// std::find_if()�� ������� �ʴ� ������ vector �ȿ� �� ���� ����ü�� ���簡 �Ͼ�� ����.
	for (auto& client : mClientInfos)
	{
		if (INVALID_SOCKET == client.m_socketClient)
		{
			return &client;
		}
	}

	return nullptr;
}

bool
IOCPServer::BindIOCompletionPort(stClientInfo* pClientInfo)
{
	//socket�� pClientInfo�� CompletionPort��ü�� �����Ų��.
	HANDLE hIOCP = ::CreateIoCompletionPort((HANDLE)pClientInfo->m_socketClient
		, mIOCPHandle
		, (ULONG_PTR)(pClientInfo), 0);

	if (NULL == hIOCP || mIOCPHandle != hIOCP)
	{
		std::cout << "[����] CreateIoCompletionPort()�Լ� ����: " << ::GetLastError() << std::endl;
		return false;
	}

	return true;
}

bool
IOCPServer::BindRecv(stClientInfo* pClientInfo)
{
	DWORD dwFlag = 0;
	DWORD dwRecvNumBytes = 0;

	//Overlapped I/O�� ���� �� ������ ������ �ش�.
	pClientInfo->m_stRecvOverlappedEx.m_wsaBuf.len = MAX_SOCKBUF;
	pClientInfo->m_stRecvOverlappedEx.m_wsaBuf.buf = pClientInfo->mRecvBuf;
	pClientInfo->m_stRecvOverlappedEx.m_eOperation = IOOperation::RECV;

	int nRet = ::WSARecv(pClientInfo->m_socketClient,
		&(pClientInfo->m_stRecvOverlappedEx.m_wsaBuf),
		1,
		&dwRecvNumBytes,
		&dwFlag,
		(LPWSAOVERLAPPED) & (pClientInfo->m_stRecvOverlappedEx),
		NULL);

	//socket_error�̸� client socket�� �������ɷ� ó���Ѵ�.
	if (nRet == SOCKET_ERROR && (::WSAGetLastError() != ERROR_IO_PENDING))
	{
		std::cerr << "[����] WSARecv()�Լ� ���� : " << WSAGetLastError() << std::endl;
		return false;
	}

	return true;
}

bool
IOCPServer::SendMsg(stClientInfo* pClientInfo, char* pMsg, int nLen)
{
	DWORD dwRecvNumBytes = 0;

	//���۵� �޼����� ����
	CopyMemory(pClientInfo->mSendBuf, pMsg, nLen);
	pClientInfo->mSendBuf[nLen] = '\0';


	//Overlapped I/O�� ���� �� ������ ������ �ش�.
	pClientInfo->m_stSendOverlappedEx.m_wsaBuf.len = nLen;
	pClientInfo->m_stSendOverlappedEx.m_wsaBuf.buf = pClientInfo->mSendBuf;
	pClientInfo->m_stSendOverlappedEx.m_eOperation = IOOperation::SEND;

	int nRet = ::WSASend(pClientInfo->m_socketClient,
		&(pClientInfo->m_stSendOverlappedEx.m_wsaBuf),
		1,
		&dwRecvNumBytes,
		0,
		(LPWSAOVERLAPPED) & (pClientInfo->m_stSendOverlappedEx),
		NULL);

	//socket_error�̸� client socket�� �������ɷ� ó���Ѵ�.
	if (nRet == SOCKET_ERROR && (::WSAGetLastError() != ERROR_IO_PENDING))
	{
		printf("[����] WSASend()�Լ� ���� : %d\n", ::WSAGetLastError());
		return false;
	}
	return true;
}

void
IOCPServer::WokerThread()
{
	//CompletionKey�� ���� ������ ����
	stClientInfo* pClientInfo = nullptr;
	//�Լ� ȣ�� ���� ����
	BOOL bSuccess = TRUE;
	//Overlapped I/O�۾����� ���۵� ������ ũ��
	DWORD dwIoSize = 0;
	//I/O �۾��� ���� ��û�� Overlapped ����ü�� ���� ������
	LPOVERLAPPED lpOverlapped = NULL;

	while (mIsWorkerRun)
	{
		bSuccess = GetQueuedCompletionStatus(mIOCPHandle,
			&dwIoSize,					// ������ ���۵� ����Ʈ
			(PULONG_PTR)&pClientInfo,		// CompletionKey
			&lpOverlapped,				// Overlapped IO ��ü
			INFINITE);					// ����� �ð�

		//����� ������ ���� �޼��� ó��..
		if (TRUE == bSuccess && 0 == dwIoSize && NULL == lpOverlapped)
		{
			mIsWorkerRun = false;
			continue;
		}

		if (NULL == lpOverlapped)
		{
			continue;
		}

		//client�� ������ ��������..			
		if (FALSE == bSuccess || (0 == dwIoSize && TRUE == bSuccess))
		{
			//printf("socket(%d) ���� ����\n", (int)pClientInfo->m_socketClient);
			CloseSocket(pClientInfo);
			continue;
		}


		stOverlappedEx* pOverlappedEx = reinterpret_cast<stOverlappedEx*>(lpOverlapped);

		//Overlapped I/O Recv�۾� ��� �� ó��
		if (IOOperation::RECV == pOverlappedEx->m_eOperation)
		{
			OnReceive(pClientInfo->mIndex, dwIoSize, pClientInfo->mRecvBuf);
			//pClientInfo->mRecvBuf[dwIoSize] = '\0';
			//printf("[����] bytes : %d , msg : %s\n", dwIoSize, pClientInfo->mRecvBuf);

			//Ŭ���̾�Ʈ�� �޼����� �����Ѵ�.
			//SendMsg(pClientInfo, pClientInfo->mRecvBuf, dwIoSize);

			BindRecv(pClientInfo);
		}
		//Overlapped I/O Send�۾� ��� �� ó��
		else if (IOOperation::SEND == pOverlappedEx->m_eOperation)
		{
			printf("[�۽�] bytes : %d , msg : %s\n", dwIoSize, pClientInfo->mSendBuf);
		}
		//���� ��Ȳ
		else
		{
			printf("socket(%d)���� ���ܻ�Ȳ\n", (int)pClientInfo->m_socketClient);
		}
	}
}

void
IOCPServer::AccepterThread()
{
	SOCKADDR_IN stClientAddr;
	int nAddrLen = sizeof(SOCKADDR_IN);

	while (mIsAccepterRun)
	{
		//������ ���� ����ü�� �ε����� ���´�.
		stClientInfo* pClientInfo = GetEmptyClientInfo();
		if (NULL == pClientInfo)
		{
			std::cerr << "[����] Client Ful" << std::endl;
			return;
		}

		//Ŭ���̾�Ʈ ���� ��û�� ���� ������ ��ٸ���.
		pClientInfo->m_socketClient = ::accept(mListenSocket, reinterpret_cast<SOCKADDR*>(&stClientAddr), &nAddrLen);
		if (INVALID_SOCKET == pClientInfo->m_socketClient)
		{
			continue;
		}

		//I/O Completion Port��ü�� ������ �����Ų��.
		bool bRet = BindIOCompletionPort(pClientInfo);
		if (false == bRet)
		{
			return;
		}

		//Recv Overlapped I/O�۾��� ��û�� ���´�.
		bRet = BindRecv(pClientInfo);
		if (false == bRet)
		{
			return;
		}

		//char clientIP[32] = { 0, };
		//inet_ntop(AF_INET, &(stClientAddr.sin_addr), clientIP, 32 - 1);
		//printf("Ŭ���̾�Ʈ ���� : IP(%s) SOCKET(%d)\n", clientIP, (int)pClientInfo->m_socketClient);

		OnConnect(pClientInfo->mIndex);

		//Ŭ���̾�Ʈ ���� ����
		++mClientCnt;
	}
}

void
IOCPServer::CloseSocket(stClientInfo* pClientInfo, bool bIsForce)
{
	INT32 clientIndex = pClientInfo->mIndex;

	struct linger stLinger = { 0, 0 };	// SO_DONTLINGER�� ����

	// bIsForce�� true�̸� SO_LINGER, timeout = 0���� �����Ͽ� ���� ���� ��Ų��. ���� : ������ �ս��� ������ ���� 
	if (true == bIsForce)
	{
		stLinger.l_onoff = 1;
	}

	//socketClose������ ������ �ۼ����� ��� �ߴ� ��Ų��.
	::shutdown(pClientInfo->m_socketClient, SD_BOTH);

	//���� �ɼ��� �����Ѵ�.
	::setsockopt(pClientInfo->m_socketClient, SOL_SOCKET, SO_LINGER, (char*)&stLinger, sizeof(stLinger));

	//���� ������ ���� ��Ų��. 
	::closesocket(pClientInfo->m_socketClient);

	pClientInfo->m_socketClient = INVALID_SOCKET;

	OnClose(clientIndex);
}