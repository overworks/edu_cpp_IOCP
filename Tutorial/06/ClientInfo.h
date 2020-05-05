#pragma once

#include "Define.h"
#include <stdio.h>
#include <mutex>
#include <queue>
#include <memory>

//클라이언트 정보를 담기위한 구조체.
class stClientInfo
{
public:
	stClientInfo(UINT index);

	UINT32 GetIndex() const			{ return mIndex; }
	bool IsConnected() const		{ return mSock != INVALID_SOCKET; }
	SOCKET GetSock() const			{ return mSock; }
	char* RecvBuffer()				{ return mRecvBuf; }
	const char* RecvBuffer() const	{ return mRecvBuf; }

	bool OnConnect(HANDLE hIOCP, SOCKET socket);
	void Close(bool bIsForce = false);
	void Clear();
	bool BindIOCompletionPort(HANDLE hIOCP);
	bool BindRecv();
	bool SendMsg(UINT32 dataSize, const char* pMsg);
	void SendCompleted(UINT32 dataSize);

private:
	bool SendIO();

private:
	INT32			mIndex = 0;
	SOCKET			mSock = INVALID_SOCKET;	// Cliet와 연결되는 소켓.
	stOverlappedEx	mRecvOverlappedEx;		// RECV Overlapped I/O작업을 위한 변수.
	
	char			mRecvBuf[MAX_SOCKBUF];	// 데이터 버퍼.

	std::mutex		mSendLock;
	std::queue<stOverlappedEx*> mSendDataQueue;
};
