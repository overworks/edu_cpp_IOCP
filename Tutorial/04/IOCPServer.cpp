#include <iostream>
#include <algorithm>
#include "IOCPServer.h"

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
		std::cerr << "[����] socket()�Լ� ���� : " << WSAGetLastError() << std::endl;
		return false;
	}

	std::cout << "���� �ʱ�ȭ ����" << std::endl;
	return true;
}

bool
IOCPServer::BindAndListen(int nBindPort)
{
	::SOCKADDR_IN stServerAddr;
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

	std::cout << "���� ��� ����.." << std::endl;
	return true;
}

bool
IOCPServer::StartServer(UINT maxClientCount)
{
	CreateClient(maxClientCount);

	//CompletionPort��ü ���� ��û�� �Ѵ�.
	mIOCPHandle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, MAX_WORKERTHREAD);
	if (NULL == mIOCPHandle)
	{
		std::cerr << "[����] CreateIoCompletionPort()�Լ� ����: " << GetLastError() << std::endl;
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

bool
IOCPServer::SendMsg(UINT32 sessionIndex, UINT32 dataSize, const char* pData)
{
	stClientInfo* pClient = GetClientInfo(sessionIndex);
	return pClient->SendMsg(dataSize, pData);
}

void
IOCPServer::DestroyThread()
{
	auto joinThread = [](std::thread& th) { if (th.joinable()) th.join(); };

	// Worker ������ ����
	mIsWorkerRun = false;
	CloseHandle(mIOCPHandle);
	std::for_each(mIOWorkerThreads.begin(), mIOWorkerThreads.end(), joinThread);
	
	// Accepter ������ ����
	mIsAccepterRun = false;
	closesocket(mListenSocket);
	joinThread(mAccepterThread);
}

void
IOCPServer::CreateClient(UINT maxClientCount)
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
		mIOWorkerThreads.emplace_back([this]() { WorkerThread(); });
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
	for (auto& client : mClientInfos)
	{
		if (!client.IsConnected())
		{
			return &client;
		}
	}

	return nullptr;
}

void
IOCPServer::WorkerThread()
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
			(PULONG_PTR)&pClientInfo,	// CompletionKey
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


		auto pOverlappedEx = (stOverlappedEx*)lpOverlapped;

		//Overlapped I/O Recv�۾� ��� �� ó��
		if (IOOperation::RECV == pOverlappedEx->m_eOperation)
		{
			OnReceive(pClientInfo->GetIndex(), dwIoSize, pClientInfo->RecvBuffer());

			pClientInfo->BindRecv();
		}
		//Overlapped I/O Send�۾� ��� �� ó��
		else if (IOOperation::SEND == pOverlappedEx->m_eOperation)
		{
			delete[] pOverlappedEx->m_wsaBuf.buf;
			delete pOverlappedEx;
			pClientInfo->SendCompleted(dwIoSize);
		}
		//���� ��Ȳ
		else
		{
			printf("Client Index(%d)���� ���ܻ�Ȳ\n", pClientInfo->GetIndex());
		}
	}
}

void
IOCPServer::AccepterThread()
{
	SOCKADDR_IN		stClientAddr;
	int nAddrLen = sizeof(SOCKADDR_IN);

	while (mIsAccepterRun)
	{
		//������ ���� ����ü�� �ε����� ���´�.
		stClientInfo* pClientInfo = GetEmptyClientInfo();
		if (NULL == pClientInfo)
		{
			printf("[����] Client Full\n");
			return;
		}


		//Ŭ���̾�Ʈ ���� ��û�� ���� ������ ��ٸ���.
		auto newSocket = accept(mListenSocket, (SOCKADDR*)&stClientAddr, &nAddrLen);
		if (INVALID_SOCKET == newSocket)
		{
			continue;
		}

		if (pClientInfo->OnConnect(mIOCPHandle, newSocket) == false)
		{
			pClientInfo->Close(true);
			return;
		}

		//char clientIP[32] = { 0, };
		//inet_ntop(AF_INET, &(stClientAddr.sin_addr), clientIP, 32 - 1);
		//printf("Ŭ���̾�Ʈ ���� : IP(%s) SOCKET(%d)\n", clientIP, (int)pClientInfo->m_socketClient);

		OnConnect(pClientInfo->GetIndex());

		//Ŭ���̾�Ʈ ���� ����
		++mClientCnt;
	}
}

void
IOCPServer::CloseSocket(stClientInfo* pClientInfo, bool bIsForce)
{
	pClientInfo->Close(bIsForce);

	OnClose(pClientInfo->GetIndex());
}