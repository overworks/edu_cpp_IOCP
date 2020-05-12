//출처: 강정중님의 저서 '온라인 게임서버'에서
#pragma once

#include <vector>
#include <thread>
#include "ClientInfo.h"
#include "Define.h"

class IOCPServer
{
public:
	IOCPServer();
	virtual ~IOCPServer();

	//소켓을 초기화하는 함수
	bool Init(UINT32 maxIOWorkerThreadCount);
		
	//서버의 주소정보를 소켓과 연결시키고 접속 요청을 받기 위해 소켓을 등록하는 함수
	bool BindAndListen(int bindPort);

	//접속 요청을 수락하고 메세지를 받아서 처리하는 함수
	bool StartServer(UINT32 maxClientCount);

	//생성되어있는 쓰레드를 파괴한다.
	void DestroyThread();

	bool SendMsg(UINT32 clientIndex, UINT32 dataSize, const char* pData);
	
protected:
	virtual void OnConnect(UINT32 clientIndex) = 0;
	virtual void OnClose(UINT32 clientIndex) = 0;
	virtual void OnReceive(UINT32 clientIndex, UINT32 size, const char* pData) = 0;

private:
	void CreateClient(const UINT32 maxClientCount_)
	{
		for (UINT32 i = 0; i < maxClientCount_; ++i)
		{
			auto client = new stClientInfo;
			client->Init(i, mIOCPHandle);

			mClientInfos.push_back(client);
		}
	}

	//WaitingThread Queue에서 대기할 쓰레드들을 생성
	bool CreateWorkerThread()
	{
		//WaingThread Queue에 대기 상태로 넣을 쓰레드들 생성 권장되는 개수 : (cpu개수 * 2) + 1 
		for (UINT32 i = 0; i < mMaxIOWorkerThreadCount; i++)
		{
			mIOWorkerThreads.emplace_back([this](){ WokerThread(); });			
		}

		printf("WokerThread 시작..\n");
		return true;
	}
	
	//사용하지 않는 클라이언트 정보 구조체를 반환한다.
	stClientInfo* GetEmptyClientInfo()
	{
		for (auto& client : mClientInfos)
		{
			if (client->IsConnectd() == false)
			{
				return client;
			}
		}

		return nullptr;
	}

	stClientInfo* GetClientInfo(UINT32 clientIndex)	{ return mClientInfos[clientIndex]; }

	//accept요청을 처리하는 쓰레드 생성
	bool CreateAccepterThread()
	{
		mAccepterThread = std::thread([this]() { AccepterThread(); });
		
		printf("AccepterThread 시작..\n");
		return true;
	}
		  		
	//Overlapped I/O작업에 대한 완료 통보를 받아 그에 해당하는 처리를 하는 함수
	void WokerThread()
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

			auto pOverlappedEx = (stOverlappedEx*)lpOverlapped;

			//client가 접속을 끊었을때..			
			if (FALSE == bSuccess || (0 == dwIoSize && IOOperation::ACCEPT != pOverlappedEx->m_eOperation))
			{
				//printf("socket(%d) 접속 끊김\n", (int)pClientInfo->m_socketClient);
				CloseSocket(pClientInfo); //Caller WokerThread()
				continue;
			}


			if (IOOperation::ACCEPT == pOverlappedEx->m_eOperation)
			{
				pClientInfo = GetClientInfo(pOverlappedEx->SessionIndex);
				if (pClientInfo->AcceptCompletion())
				{
					//클라이언트 갯수 증가
					++mClientCnt;		

					OnConnect(pClientInfo->GetIndex());
				}
				else
				{
					CloseSocket(pClientInfo, true);  //Caller WokerThread()
				}
			}
			//Overlapped I/O Recv작업 결과 뒤 처리
			else if (IOOperation::RECV == pOverlappedEx->m_eOperation)
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
				printf("Client Index(%d)에서 예외상황\n", pClientInfo->GetIndex());
			}
		}
	}

	//사용자의 접속을 받는 쓰레드
	void AccepterThread()
	{
		while (mIsAccepterRun)
		{
			auto curTimeSec = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

			for (auto client : mClientInfos)
			{
				if (client->IsConnectd())
				{
					continue;
				}

				if ((UINT64)curTimeSec < client->GetLatestClosedTimeSec())
				{
					continue;
				}

				auto diff = curTimeSec - client->GetLatestClosedTimeSec();
				if (diff <= RE_USE_SESSION_WAIT_TIMESEC)
				{
					continue;
				}

				client->PostAccept(mListenSocket, curTimeSec);
			}
			
			std::this_thread::sleep_for(std::chrono::milliseconds(32));
		}
	}
	
	//소켓의 연결을 종료 시킨다.
	void CloseSocket(stClientInfo* clientInfo_, bool isForce_ = false)
	{
		if (clientInfo_->IsConnectd() == false)
		{
			return;
		}

		auto clientIndex = clientInfo_->GetIndex();

		clientInfo_->Close(isForce_);
		
		OnClose(clientIndex);
	}


private:
	UINT32 mMaxIOWorkerThreadCount = 0;

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