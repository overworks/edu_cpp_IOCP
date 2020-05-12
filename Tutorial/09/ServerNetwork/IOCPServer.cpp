#include <iostream>
#include <algorithm>
#include "IOCPServer.h"

IOCPServer::IOCPServer()
{

}

IOCPServer::~IOCPServer()
{
	//윈속의 사용을 끝낸다.
	::WSACleanup();
}

bool
IOCPServer::Init(UINT32 maxIOWorkerThreadCount)
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

	mMaxIOWorkerThreadCount = maxIOWorkerThreadCount;

	std::cout << u8"소켓 초기화 성공" << std::endl;
	return true;
}

bool
IOCPServer::BindAndListen(int bindPort)
{
	::SOCKADDR_IN stServerAddr;
	stServerAddr.sin_family = AF_INET;
	stServerAddr.sin_port = ::htons(bindPort); //서버 포트를 설정한다.		
	//어떤 주소에서 들어오는 접속이라도 받아들이겠다.
	//보통 서버라면 이렇게 설정한다. 만약 한 아이피에서만 접속을 받고 싶다면
	//그 주소를 inet_addr함수를 이용해 넣으면 된다.
	stServerAddr.sin_addr.s_addr = ::htonl(INADDR_ANY);

	//위에서 지정한 서버 주소 정보와 cIOCompletionPort 소켓을 연결한다.
	int nRet = ::bind(mListenSocket, (SOCKADDR*)&stServerAddr, sizeof(SOCKADDR_IN));
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

	//CompletionPort객체 생성 요청을 한다.
	mIOCPHandle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, mMaxIOWorkerThreadCount);
	if (NULL == mIOCPHandle)
	{
		std::cerr << u8"[에러] CreateIoCompletionPort()함수 실패: " << ::GetLastError() << std::endl;
		return false;
	}

	HANDLE hIOCPHandle = ::CreateIoCompletionPort((HANDLE)mListenSocket, mIOCPHandle, (UINT32)0, 0);
	if (NULL == hIOCPHandle)
	{
		std::cerr << u8"[에러] listen socket IOCP bind 실패 : " << ::WSAGetLastError() << std::endl;
		return false;
	}

	std::cout << u8"서버 등록 성공.." << std::endl;
	return true;
}

bool
IOCPServer::StartServer(UINT32 maxClientCount)
{
	CreateClient(maxClientCount);

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

void
IOCPServer::DestroyThread()
{
	auto joinThread = [](std::thread& th) { if (th.joinable()) th.join(); };

	mIsWorkerRun = false;
	CloseHandle(mIOCPHandle);
	std::for_each(mIOWorkerThreads.begin(), mIOWorkerThreads.end(), joinThread);
	
	//Accepter 쓰레드를 종요한다.
	mIsAccepterRun = false;
	closesocket(mListenSocket);
	joinThread(mAccepterThread);
}

bool
IOCPServer::SendMsg(UINT32 clientIndex, UINT32 dataSize, const char* pData)
{
	return GetClientInfo(clientIndex)->SendMsg(dataSize, pData);
}