#include <iostream>
#include <algorithm>
#include "IOCompletionPort.h"

// stClientInfo
stClientInfo::stClientInfo()
{
	m_socketClient = INVALID_SOCKET; 
	ZeroMemory(&m_stRecvOverlappedEx, sizeof(stOverlappedEx));
	ZeroMemory(&m_stSendOverlappedEx, sizeof(stOverlappedEx));
}

// 이하 IOCompletionPort 구현
IOCompletionPort::IOCompletionPort()
{

}

IOCompletionPort::~IOCompletionPort()
{
	//윈속의 사용을 끝낸다.
	::WSACleanup();
}

bool
IOCompletionPort::InitSocket()
{
	WSADATA wsaData;

	int nRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (0 != nRet)
	{
		std::cerr << "[에러] WSAStartup()함수 실패 : " << WSAGetLastError() << std::endl;
		return false;
	}

	//연결지향형 TCP , Overlapped I/O 소켓을 생성
	mListenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);

	if (INVALID_SOCKET == mListenSocket)
	{
		std::cerr << "[에러] socket()함수 실패 : " << WSAGetLastError() << std::endl;
		return false;
	}

	std::cout << "소켓 초기화 성공" << std::endl;
	return true;
}

bool
IOCompletionPort::BindAndListen(int nBindPort)
{
	SOCKADDR_IN		stServerAddr;
	stServerAddr.sin_family = AF_INET;
	stServerAddr.sin_port = htons(nBindPort); //서버 포트를 설정한다.		
	//어떤 주소에서 들어오는 접속이라도 받아들이겠다.
	//보통 서버라면 이렇게 설정한다. 만약 한 아이피에서만 접속을 받고 싶다면
	//그 주소를 inet_addr함수를 이용해 넣으면 된다.
	stServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	//위에서 지정한 서버 주소 정보와 cIOCompletionPort 소켓을 연결한다.
	int nRet = bind(mListenSocket, (SOCKADDR*)&stServerAddr, sizeof(SOCKADDR_IN));
	if (0 != nRet)
	{
		std::cerr << "[에러] bind()함수 실패 : " << WSAGetLastError() << std::endl;
		return false;
	}

	//접속 요청을 받아들이기 위해 cIOCompletionPort소켓을 등록하고 
	//접속대기큐를 5개로 설정 한다.
	nRet = listen(mListenSocket, 5);
	if (0 != nRet)
	{
		std::cerr << "[에러] listen()함수 실패 : " << WSAGetLastError() << std::endl;
		return false;
	}

	std::cout << "서버 등록 성공.." << std::endl;
	return true;
}

bool
IOCompletionPort::StartServer(size_t maxClientCount)
{
	CreateClient(maxClientCount);

	//CompletionPort객체 생성 요청을 한다.
	mIOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, MAX_WORKERTHREAD);
	if (NULL == mIOCPHandle)
	{
		std::cerr << "[에러] CreateIoCompletionPort()함수 실패: " << GetLastError() << std::endl;
		return false;
	}

	//접속된 클라이언트 주소 정보를 저장할 구조체
	bool bRet = CreateWokerThread();
	if (false == bRet) {
		return false;
	}

	bRet = CreateAccepterThread();
	if (false == bRet) {
		return false;
	}

	std::cout << "서버 시작" << std::endl;;
	return true;
}

bool
IOCompletionPort::CreateWokerThread()
{
	//WaingThread Queue에 대기 상태로 넣을 쓰레드들 생성 권장되는 개수 : (cpu개수 * 2) + 1 
	for (int i = 0; i < MAX_WORKERTHREAD; i++)
	{
		mIOWorkerThreads.emplace_back([this]() { WorkerThread(); });
	}

	std::cout << "WokerThread 시작.." << std::endl;
	return true;
}

bool
IOCompletionPort::CreateAccepterThread()
{
	mAccepterThread = std::thread([this]() { AccepterThread(); });

	std::cout << "AccepterThread 시작.." << std::endl;
	return true;
}

void
IOCompletionPort::CreateClient(UINT32 maxClientCount)
{
	for (UINT32 i = 0; i < maxClientCount; ++i)
	{
		mClientInfos.emplace_back();
	}
}

stClientInfo*
IOCompletionPort::GetEmptyClientInfo()
{
	auto iter = std::find_if(mClientInfos.begin(), mClientInfos.end(), [](auto& client) {
		return INVALID_SOCKET == client.m_socketClient;
	});
	return iter != mClientInfos.end() ? &(*iter) : nullptr;
}

bool
IOCompletionPort::BindIOCompletionPort(stClientInfo* pClientInfo)
{
	//socket과 pClientInfo를 CompletionPort객체와 연결시킨다.
	HANDLE hIOCP = CreateIoCompletionPort((HANDLE)pClientInfo->m_socketClient
		, mIOCPHandle
		, (ULONG_PTR)(pClientInfo), 0);

	if (NULL == hIOCP || mIOCPHandle != hIOCP)
	{
		std::cerr << "[에러] CreateIoCompletionPort()함수 실패: " << GetLastError() << std::endl;
		return false;
	}

	return true;
}

bool
IOCompletionPort::BindRecv(stClientInfo* pClientInfo)
{
	DWORD dwFlag = 0;
	DWORD dwRecvNumBytes = 0;

	//Overlapped I/O을 위해 각 정보를 셋팅해 준다.
	pClientInfo->m_stRecvOverlappedEx.m_wsaBuf.len = MAX_SOCKBUF;
	pClientInfo->m_stRecvOverlappedEx.m_wsaBuf.buf = pClientInfo->m_stRecvOverlappedEx.m_szBuf;
	pClientInfo->m_stRecvOverlappedEx.m_eOperation = IOOperation::RECV;

	int nRet = ::WSARecv(pClientInfo->m_socketClient,
		&(pClientInfo->m_stRecvOverlappedEx.m_wsaBuf),
		1,
		&dwRecvNumBytes,
		&dwFlag,
		(LPWSAOVERLAPPED) & (pClientInfo->m_stRecvOverlappedEx),
		NULL);

	//socket_error이면 client socket이 끊어진걸로 처리한다.
	if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
	{
		std::cerr << "[에러] WSARecv()함수 실패 : " << WSAGetLastError() << std::endl;
		return false;
	}

	return true;
}

bool
IOCompletionPort::SendMsg(stClientInfo* pClientInfo, char* pMsg, int nLen)
{
	DWORD dwRecvNumBytes = 0;

	//전송될 메세지를 복사
	CopyMemory(pClientInfo->m_stSendOverlappedEx.m_szBuf, pMsg, nLen);

	//Overlapped I/O을 위해 각 정보를 셋팅해 준다.
	pClientInfo->m_stSendOverlappedEx.m_wsaBuf.len = nLen;
	pClientInfo->m_stSendOverlappedEx.m_wsaBuf.buf = pClientInfo->m_stSendOverlappedEx.m_szBuf;
	pClientInfo->m_stSendOverlappedEx.m_eOperation = IOOperation::SEND;

	int nRet = ::WSASend(pClientInfo->m_socketClient,
		&(pClientInfo->m_stSendOverlappedEx.m_wsaBuf),
		1,
		&dwRecvNumBytes,
		0,
		(LPWSAOVERLAPPED) & (pClientInfo->m_stSendOverlappedEx),
		NULL);

	//socket_error이면 client socket이 끊어진걸로 처리한다.
	if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
	{
		std::cerr << "[에러] WSASend()함수 실패 : " << WSAGetLastError() << std::endl;
		return false;
	}
	return true;
}

void
IOCompletionPort::WorkerThread()
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
		//////////////////////////////////////////////////////
		//이 함수로 인해 쓰레드들은 WaitingThread Queue에
		//대기 상태로 들어가게 된다.
		//완료된 Overlapped I/O작업이 발생하면 IOCP Queue에서
		//완료된 작업을 가져와 뒤 처리를 한다.
		//그리고 PostQueuedCompletionStatus()함수에의해 사용자
		//메세지가 도착되면 쓰레드를 종료한다.
		//////////////////////////////////////////////////////
		bSuccess = GetQueuedCompletionStatus(mIOCPHandle,
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
			std::cout << "socket(" << pClientInfo->m_socketClient << ") 접속 끊김" << std::endl;
			CloseSocket(pClientInfo);
			continue;
		}


		stOverlappedEx* pOverlappedEx = reinterpret_cast<stOverlappedEx*>(lpOverlapped);

		//Overlapped I/O Recv작업 결과 뒤 처리
		if (IOOperation::RECV == pOverlappedEx->m_eOperation)
		{
			pOverlappedEx->m_szBuf[dwIoSize] = NULL;
			printf("[수신] bytes : %d , msg : %s\n", dwIoSize, pOverlappedEx->m_szBuf);

			//클라이언트에 메세지를 에코한다.
			SendMsg(pClientInfo, pOverlappedEx->m_szBuf, dwIoSize);
			BindRecv(pClientInfo);
		}
		//Overlapped I/O Send작업 결과 뒤 처리
		else if (IOOperation::SEND == pOverlappedEx->m_eOperation)
		{
			printf("[송신] bytes : %d , msg : %s\n", dwIoSize, pOverlappedEx->m_szBuf);
		}
		//예외 상황
		else
		{
			printf("socket(%d)에서 예외상황\n", (int)pClientInfo->m_socketClient);
		}
	}
}

void
IOCompletionPort::AccepterThread()
{
	SOCKADDR_IN		stClientAddr;
	int nAddrLen = sizeof(SOCKADDR_IN);

	while (mIsAccepterRun)
	{
		//접속을 받을 구조체의 인덱스를 얻어온다.
		stClientInfo* pClientInfo = GetEmptyClientInfo();
		if (nullptr == pClientInfo)
		{
			std::cerr << "[에러] Client Full\n" << std::endl;
			return;
		}

		//클라이언트 접속 요청이 들어올 때까지 기다린다.
		pClientInfo->m_socketClient = accept(mListenSocket, (SOCKADDR*)&stClientAddr, &nAddrLen);
		if (INVALID_SOCKET == pClientInfo->m_socketClient)
		{
			continue;
		}

		//I/O Completion Port객체와 소켓을 연결시킨다.
		bool bRet = BindIOCompletionPort(pClientInfo);
		if (false == bRet)
		{
			return;
		}

		//Recv Overlapped I/O작업을 요청해 놓는다.
		bRet = BindRecv(pClientInfo);
		if (false == bRet)
		{
			return;
		}

		char clientIP[32] = { 0, };
		inet_ntop(AF_INET, &(stClientAddr.sin_addr), clientIP, 32 - 1);
		printf("클라이언트 접속 : IP(%s) SOCKET(%d)\n", clientIP, (int)pClientInfo->m_socketClient);

		//클라이언트 갯수 증가
		++mClientCnt;
	}
}

void
IOCompletionPort::CloseSocket(stClientInfo* pClientInfo, bool bIsForce)
{
	struct linger stLinger = { 0, 0 };	// SO_DONTLINGER로 설정

	// bIsForce가 true이면 SO_LINGER, timeout = 0으로 설정하여 강제 종료 시킨다. 주의 : 데이터 손실이 있을수 있음 
	if (true == bIsForce)
	{
		stLinger.l_onoff = 1;
	}

	//socketClose소켓의 데이터 송수신을 모두 중단 시킨다.
	::shutdown(pClientInfo->m_socketClient, SD_BOTH);

	//소켓 옵션을 설정한다.
	::setsockopt(pClientInfo->m_socketClient, SOL_SOCKET, SO_LINGER, (char*)&stLinger, sizeof(stLinger));

	//소켓 연결을 종료 시킨다. 
	::closesocket(pClientInfo->m_socketClient);

	pClientInfo->m_socketClient = INVALID_SOCKET;
}


void
IOCompletionPort::DestroyThread()
{
	mIsWorkerRun = false;
	CloseHandle(mIOCPHandle);

	for (auto& th : mIOWorkerThreads)
	{
		if (th.joinable())
		{
			th.join();
		}
	}

	//Accepter 쓰레드를 종요한다.
	mIsAccepterRun = false;
	closesocket(mListenSocket);

	if (mAccepterThread.joinable())
	{
		mAccepterThread.join();
	}
}
