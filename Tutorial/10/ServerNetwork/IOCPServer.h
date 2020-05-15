//출처: 강정중님의 저서 '온라인 게임서버'에서
#include <cstdint>
#include <thread>
#include <vector>
#include "ClientInfo.h"
#include "Define.h"

class IOCPServer
{
public:
	IOCPServer();
	virtual ~IOCPServer();

	//소켓을 초기화하는 함수
	bool Init(uint32_t maxIOWorkerThreadCount);

	//서버의 주소정보를 소켓과 연결시키고 접속 요청을 받기 위해 소켓을 등록하는 함수
	bool BindAndListen(int bindPort);

	//접속 요청을 수락하고 메세지를 받아서 처리하는 함수
	bool StartServer(int maxClientCount);

	//생성되어있는 쓰레드를 파괴한다.
	void DestroyThread();

	bool SendMsg(int clientIndex, uint32_t dataSize, const char* pData);
	
protected:
	virtual void OnConnect(uint32_t clientIndex) = 0;
	virtual void OnClose(uint32_t clientIndex) = 0;
	virtual void OnReceive(uint32_t clientIndex, uint32_t size, const char* pData_) = 0;

private:
	stClientInfo* GetClientInfo(int clientIndex) { return mClientInfos[clientIndex]; }
	const stClientInfo* GetClientInfo(int clientIndex) const { return mClientInfos[clientIndex]; }
	
	//사용하지 않는 클라이언트 정보 구조체를 반환한다.
	stClientInfo* GetEmptyClientInfo();

	void CreateClient(int maxClientCount);

	//WaitingThread Queue에서 대기할 쓰레드들을 생성
	bool CreateWorkerThread();
	
	//accept요청을 처리하는 쓰레드 생성
	bool CreateAccepterThread();
		  		
	//Overlapped I/O작업에 대한 완료 통보를 받아 그에 해당하는 처리를 하는 함수
	void WorkerThread();

	//사용자의 접속을 받는 쓰레드
	void AccepterThread();
	
	//소켓의 연결을 종료 시킨다.
	void CloseSocket(stClientInfo* clientInfo, bool isForce = false);

private:
	uint32_t mMaxIOWorkerThreadCount = 0;

	//클라이언트 정보 저장 구조체
	std::vector<stClientInfo*> mClientInfos;

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
};