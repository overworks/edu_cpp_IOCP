#pragma once

#include "Define.h"
#include <stdio.h>

//클라이언트 정보를 담기위한 구조체
class stClientInfo
{
public:
	stClientInfo(UINT index);

	UINT32 GetIndex() const			{ return mIndex; }
	bool IsConnected() const		{ return mSock != INVALID_SOCKET; }
	SOCKET GetSock() const			{ return mSock; }
	char* RecvBuffer()				{ return mRecvBuf; }
	const char* RecvBuffer() const	{ return mRecvBuf; }

	bool OnConnect(HANDLE iocpHandle, SOCKET socket);
	void Close(bool bIsForce = false);
	void Clear();
	bool BindIOCompletionPort(HANDLE hIOCP);
	bool BindRecv();

	// 1개의 스레드에서만 호출해야 한다!
	bool SendMsg(UINT32 dataSize, const char* pMsg) const;
	void SendCompleted(UINT32 dataSize);

private:
	INT32			mIndex;
	SOCKET			mSock;				//Cliet와 연결되는 소켓
	stOverlappedEx	mRecvOverlappedEx;	//RECV Overlapped I/O작업을 위한 변수
	stOverlappedEx	mSendOverlappedEx;
	
	char			mRecvBuf[MAX_SOCKBUF]; //데이터 버퍼
	mutable char	mSendBuf[MAX_SOCKBUF];

	//std::queue<stOverlappedEx*> mSendDataqueue;
};