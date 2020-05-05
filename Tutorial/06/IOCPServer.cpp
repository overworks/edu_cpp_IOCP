#include <iostream>
#include <algorithm>
#include "IOCPServer.h"

IOCPServer::~IOCPServer()
{
	std::for_each(mClientInfos.begin(), mClientInfos.end(), [](auto client) { delete client; });
	mClientInfos.clear();

	//윈속의 사용을 끝낸다.
	::WSACleanup();
}

bool
IOCPServer::InitSocket()
{
	::WSADATA wsaData;

	int nRet = ::WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (0 != nRet)
	{
		std::cerr << u8"[에러] WSAStartup()함수 실패 : " << ::WSAGetLastError() << std::endl;
		return false;
	}

	//연결지향형 TCP , Overlapped I/O 소켓을 생성
	mListenSocket = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);

	if (INVALID_SOCKET == mListenSocket)
	{
		std::cerr << u8"[에러] socket()함수 실패 : " << ::WSAGetLastError() << std::endl;
		return false;
	}

	std::cout << u8"소켓 초기화 성공" << std::endl;
	return true;
}

bool
IOCPServer::BindAndListen(int nBindPort)
{
	::SOCKADDR_IN	stServerAddr;
	stServerAddr.sin_family = AF_INET;
	stServerAddr.sin_port = htons(nBindPort); //서버 포트를 설정한다.		
	//어떤 주소에서 들어오는 접속이라도 받아들이겠다.
	//보통 서버라면 이렇게 설정한다. 만약 한 아이피에서만 접속을 받고 싶다면
	//그 주소를 inet_addr함수를 이용해 넣으면 된다.
	stServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	//위에서 지정한 서버 주소 정보와 cIOCompletionPort 소켓을 연결한다.
	int nRet = ::bind(mListenSocket, (::SOCKADDR*)&stServerAddr, sizeof(SOCKADDR_IN));
	if (0 != nRet)
	{
		std::cerr << u8"[에러] bind()함수 실패 : " << ::WSAGetLastError() << std::endl;
		return false;
	}

	//접속 요청을 받아들이기 위해 cIOCompletionPort소켓을 등록하고 
	//접속대기큐를 5개로 설정 한다.
	nRet = ::listen(mListenSocket, 5);
	if (0 != nRet)
	{
		std::cerr << u8"[에러] listen()함수 실패 : " << ::WSAGetLastError() << std::endl;
		return false;
	}

	std::cout << u8"서버 등록 성공.." << std::endl;
	return true;
}

bool
IOCPServer::StartServer(UINT32 maxClientCount)
{
	CreateClient(maxClientCount);

	//CompletionPort객체 생성 요청을 한다.
	mIOCPHandle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, MAX_WORKERTHREAD);
	if (NULL == mIOCPHandle)
	{
		std::cerr << u8"[에러] CreateIoCompletionPort()함수 실패: " << ::GetLastError() << std::endl;
		return false;
	}

	//접속된 클라이언트 주소 정보를 저장할 구조체
	bool bRet = CreateWorkerThread();
	if (false == bRet) {
		return false;
	}

	bRet = CreateAccepterThread();
	if (false == bRet) {
		return false;
	}

	std::cout << u8"서버 시작" << std::endl;
	return true;
}

stClientInfo*
IOCPServer::GetEmptyClientInfo()
{
	auto iter = std::find_if(mClientInfos.begin(), mClientInfos.end(), [](auto client) { return !client->IsConnected(); });
	return iter != mClientInfos.end() ? *iter : nullptr;
}

void
IOCPServer::CreateClient(UINT32 maxClientCount)
{
	for (UINT32 i = 0; i < maxClientCount; ++i)
	{
		auto client = new stClientInfo(i);

		mClientInfos.push_back(client);
	}
}

bool
IOCPServer::CreateWorkerThread()
{
	//WaingThread Queue에 대기 상태로 넣을 쓰레드들 생성 권장되는 개수 : (cpu개수 * 2) + 1 
	for (int i = 0; i < MAX_WORKERTHREAD; i++)
	{
		mIOWorkerThreads.emplace_back([this]() { WorkerThread(); });
	}

	std::cout << u8"WokerThread 시작.." << std::endl;
	return true;
}

bool
IOCPServer::CreateAccepterThread()
{
	mAccepterThread = std::thread([this]() { AccepterThread(); });

	std::cout << u8"AccepterThread 시작.." << std::endl;
	return true;
}

void
IOCPServer::DestroyThread()
{
	auto joinThread = [](std::thread& th) { if (th.joinable()) th.join(); };

	// Worker 스레드 종료
	mIsWorkerRun = false;
	::CloseHandle(mIOCPHandle);
	std::for_each(mIOWorkerThreads.begin(), mIOWorkerThreads.end(), joinThread);

	// Accepter 스레드 종료
	mIsAccepterRun = false;
	::closesocket(mListenSocket);
	joinThread(mAccepterThread);
}

void
IOCPServer::WorkerThread()
{
	//CompletionKey를 받을 포인터 변수
	stClientInfo* pClientInfo = nullptr;
	//함수 호출 성공 여부
	BOOL bSuccess = TRUE;
	//Overlapped I/O작업에서 전송된 데이터 크기
	DWORD dwIoSize = 0;
	//I/O 작업을 위해 요청한 Overlapped 구조체를 받을 포인터
	LPOVERLAPPED lpOverlapped = NULL;

	while (mIsWorkerRun)
	{
		bSuccess = ::GetQueuedCompletionStatus(mIOCPHandle,
			&dwIoSize,					// 실제로 전송된 바이트
			(PULONG_PTR)&pClientInfo,		// CompletionKey
			&lpOverlapped,				// Overlapped IO 객체
			INFINITE);					// 대기할 시간

		//사용자 쓰레드 종료 메세지 처리..
		if (TRUE == bSuccess && 0 == dwIoSize && NULL == lpOverlapped)
		{
			mIsWorkerRun = false;
			continue;
		}

		if (NULL == lpOverlapped)
		{
			continue;
		}

		//client가 접속을 끊었을때..			
		if (FALSE == bSuccess || (0 == dwIoSize && TRUE == bSuccess))
		{
			//printf("socket(%d) 접속 끊김\n", (int)pClientInfo->m_socketClient);
			CloseSocket(pClientInfo);
			continue;
		}


		auto pOverlappedEx = (stOverlappedEx*)lpOverlapped;

		//Overlapped I/O Recv작업 결과 뒤 처리
		if (IOOperation::RECV == pOverlappedEx->m_eOperation)
		{
			OnReceive(pClientInfo->GetIndex(), dwIoSize, pClientInfo->RecvBuffer());

			pClientInfo->BindRecv();
		}
		//Overlapped I/O Send작업 결과 뒤 처리
		else if (IOOperation::SEND == pOverlappedEx->m_eOperation)
		{
			pClientInfo->SendCompleted(dwIoSize);
		}
		//예외 상황
		else
		{
			printf(u8"Client Index(%d)에서 예외상황\n", pClientInfo->GetIndex());
		}
	}
}

void
IOCPServer::AccepterThread()
{
	::SOCKADDR_IN	stClientAddr;
	int nAddrLen = sizeof(::SOCKADDR_IN);

	while (mIsAccepterRun)
	{
		//접속을 받을 구조체의 인덱스를 얻어온다.
		stClientInfo* pClientInfo = GetEmptyClientInfo();
		if (NULL == pClientInfo)
		{
			std::cout << u8"[에러] Client Full " << std::endl;
			return;
		}

		//클라이언트 접속 요청이 들어올 때까지 기다린다.
		SOCKET newSocket = ::accept(mListenSocket, (SOCKADDR*)&stClientAddr, &nAddrLen);
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
		//printf("클라이언트 접속 : IP(%s) SOCKET(%d)\n", clientIP, (int)pClientInfo->m_socketClient);

		OnConnect(pClientInfo->GetIndex());

		//클라이언트 갯수 증가
		++mClientCnt;
	}
}

void
IOCPServer::CloseSocket(stClientInfo* pClientInfo, bool bIsForce)
{
	pClientInfo->Close(bIsForce);

	OnClose(pClientInfo->GetIndex());
}