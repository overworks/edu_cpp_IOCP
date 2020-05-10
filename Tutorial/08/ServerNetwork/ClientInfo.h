#pragma once

#include "Define.h"
#include <stdio.h>
#include <mutex>
#include <queue>


//클라이언트 정보를 담기위한 구조체
class stClientInfo
{
public:
	stClientInfo(int index, HANDLE hIOCP);

	int GetIndex() const { return mIndex; }
	bool IsConnected() const { return mIsConnect; }
	SOCKET GetSock() const { return mSocket; }
	UINT64 GetLatestClosedTimeSec() { return mLatestClosedTimeSec; }
	char* RecvBuffer() { return mRecvBuf; }

	bool OnConnect(HANDLE hIOCP, SOCKET socket);
	void Close(bool bIsForce = false);
	void Clear();
	bool PostAccept(SOCKET listenSock, uint64_t curTimeSec);
	bool AcceptCompletion();
	bool BindIOCompletionPort(HANDLE hIOCP);
	bool BindRecv();

	// 1개의 스레드에서만 호출해야 한다!
	bool SendMsg(UINT32 dataSize, const char* pMsg);

	void SendCompleted(UINT32 dataSize);

private:
	bool SendIO();
	bool SetSocketOption();

private:
	int mIndex = 0;
	HANDLE mIOCPHandle = INVALID_HANDLE_VALUE;

	bool mIsConnect = false;
	UINT64 mLatestClosedTimeSec = 0;

	SOCKET			mSocket = INVALID_SOCKET;			//Cliet와 연결되는 소켓

	stOverlappedEx	mAcceptContext;
	char			mAcceptBuf[64];

	stOverlappedEx	mRecvOverlappedEx;	//RECV Overlapped I/O작업을 위한 변수	
	char			mRecvBuf[MAX_SOCK_RECVBUF]; //데이터 버퍼

	std::mutex		mSendLock;
	std::queue<stOverlappedEx*> mSendDataqueue;
};