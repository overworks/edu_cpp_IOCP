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
	mListenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);

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
	int nRet = ::bind(mListenSocket, (::SOCKADDR*)&stServerAddr, sizeof(::SOCKADDR_IN));
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
		return false;
	}

	bRet = CreateAccepterThread();
	if (false == bRet) {
		return false;
	}

	CreateSendThread();

	std::cout << "���� ����" << std::endl;
	return true;
}

void
IOCPServer::CreateClient(UINT32 maxClientCount)
{
	for (UINT32 i = 0; i < maxClientCount; ++i)
	{
		mClientInfos.emplace_back(new stClientInfo(i));
	}
}

bool
IOCPServer::CreateWorkerThread()
{
	mIsWorkerRun = true;

	//WaingThread Queue�� ��� ���·� ���� ������� ���� ����Ǵ� ���� : (cpu���� * 2) + 1 
	for (int i = 0; i < MAX_WORKERTHREAD; i++)
	{
		mIOWorkerThreads.emplace_back([this]() { WorkProcess(); });
	}

	std::cout << "WokerThread ����.." << std::endl;
	return true;
}

bool
IOCPServer::CreateAccepterThread()
{
	mIsAccepterRun = true;
	mAccepterThread = std::thread([this]() { AcceptProcess(); });

	std::cout << "AccepterThread ����.." << std::endl;
	return true;
}

void
IOCPServer::CreateSendThread()
{
	mIsSenderRun = true;
	mSendThread = std::thread([this]() { SendProcess(); });
	std::cout << "SendThread ����.." << std::endl;
}

void
IOCPServer::DestroyThread()
{
	auto joinThread = [](std::thread& th) { if (th.joinable()) th.join(); };

	mIsSenderRun = false;
	joinThread(mSendThread);
	
	mIsAccepterRun = false;
	closesocket(mListenSocket);
	joinThread(mAccepterThread);
	
	mIsWorkerRun = false;
	CloseHandle(mIOCPHandle);
	std::for_each(mIOWorkerThreads.begin(), mIOWorkerThreads.end(), joinThread);
}

ClientInfoPtr
IOCPServer::GetEmptyClientInfo()
{
	auto iter = std::find_if(mClientInfos.begin(), mClientInfos.end(), [](auto client) { return !client->IsConnected(); });
	return iter != mClientInfos.end() ? *iter : nullptr;
}

bool
IOCPServer::SendMsg(UINT32 sessionIndex, UINT32 dataSize, char* pData)
{
	auto pClient = GetClientInfo(sessionIndex);
	return pClient->SendMsg(dataSize, pData);
}

void
IOCPServer::CloseSocket(ClientInfoPtr pClientInfo, bool bIsForce)
{
	pClientInfo->Close(bIsForce);
	OnClose(pClientInfo->GetIndex());
}

void
IOCPServer::WorkProcess()
{
	//CompletionKey�� ���� ������ ����
	ClientInfoPtr pClientInfo = nullptr;
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
IOCPServer::AcceptProcess()
{
	SOCKADDR_IN		stClientAddr;
	int nAddrLen = sizeof(SOCKADDR_IN);

	while (mIsAccepterRun)
	{
		//������ ���� ����ü�� �ε����� ���´�.
		ClientInfoPtr pClientInfo = GetEmptyClientInfo();
		if (!pClientInfo)
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
IOCPServer::SendProcess()
{
	while (mIsSenderRun)
	{
		std::for_each(mClientInfos.begin(), mClientInfos.end(), [](ClientInfoPtr client) {
			if (client->IsConnected())
			{
				client->SendIO();
			}
		});
		std::this_thread::sleep_for(std::chrono::milliseconds(8));
	}
}