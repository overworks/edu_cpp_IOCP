//출처: 강정중님의 저서 '온라인 게임서버'에서
#pragma once

#include "Define.h"
#include <thread>
#include <vector>

class IOCompletionPort
{
public:
	IOCompletionPort();
	~IOCompletionPort();

	//소켓을 초기화하는 함수
	bool InitSocket();

	
	//------서버용 함수-------//
	//서버의 주소정보를 소켓과 연결시키고 접속 요청을 받기 위해 
	//소켓을 등록하는 함수
	bool BindAndListen(int nBindPort);

	//접속 요청을 수락하고 메세지를 받아서 처리하는 함수
	bool StartServer(UINT32 maxClientCount);

	//생성되어있는 쓰레드를 파괴한다.
	void DestroyThread();


private:
	void CreateClient(UINT32 maxClientCount);

	//WaitingThread Queue에서 대기할 쓰레드들을 생성
	bool CreateWokerThread();
	
	//accept요청을 처리하는 쓰레드 생성
	bool CreateAccepterThread();

	//사용하지 않는 클라이언트 정보 구조체를 반환한다.
	stClientInfo* GetEmptyClientInfo();

	//CompletionPort객체와 소켓과 CompletionKey를
	//연결시키는 역할을 한다.
	bool BindIOCompletionPort(stClientInfo* pClientInfo);
  	
	//WSARecv Overlapped I/O 작업을 시킨다.
	bool BindRecv(stClientInfo* pClientInfo);

	//WSASend Overlapped I/O작업을 시킨다.
	bool SendMsg(stClientInfo* pClientInfo, char* pMsg, int nLen);

	//Overlapped I/O작업에 대한 완료 통보를 받아 
	//그에 해당하는 처리를 하는 함수
	void WorkerThread();

	//사용자의 접속을 받는 쓰레드
	void AccepterThread();

	//소켓의 연결을 종료 시킨다.
	void CloseSocket(stClientInfo* pClientInfo, bool bIsForce = false);

private:
	//클라이언트 정보 저장 구조체
	std::vector<stClientInfo> mClientInfos;

	//클라이언트의 접속을 받기위한 리슨 소켓
	SOCKET		mListenSocket = INVALID_SOCKET;
	
	//접속 되어있는 클라이언트 수
	int			mClientCnt = 0;
	
	//IO Worker 스레드
	std::vector<std::thread> mIOWorkerThreads;

	//Accept 스레드
	std::thread	mAccepterThread;

	//CompletionPort객체 핸들
	HANDLE		mIOCPHandle = INVALID_HANDLE_VALUE;
	
	//작업 쓰레드 동작 플래그
	bool		mIsWorkerRun = true;

	//접속 쓰레드 동작 플래그
	bool		mIsAccepterRun = true;
	//소켓 버퍼
	char		mSocketBuf[1024] = { 0, };
};
