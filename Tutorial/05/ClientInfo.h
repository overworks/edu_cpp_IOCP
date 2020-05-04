#pragma once

#include "Define.h"
#include <stdio.h>
#include <mutex>

//클라이언트 정보를 담기위한 구조체
class stClientInfo
{
public:
	stClientInfo(UINT index);
	~stClientInfo() = default;

	UINT32 GetIndex() const			{ return mIndex; }
	bool IsConnected() const		{ return mSock != INVALID_SOCKET; }
	SOCKET GetSock() const			{ return mSock; }
	char* RecvBuffer()				{ return mRecvBuf; }
	const char* RecvBuffer() const	{ return mRecvBuf; }


	bool OnConnect(HANDLE iocpHandle_, SOCKET socket_);
	void Close(bool bIsForce = false);
	void Clear();
	bool BindIOCompletionPort(HANDLE hIOCP);
	bool BindRecv();

	// 1개의 스레드에서만 호출해야 한다!
	bool SendMsg(UINT32 dataSize, const char* pMsg);
	bool SendIO();
	void SendCompleted(UINT32 dataSize_);

private:
	UINT32			mIndex = 0;
	SOCKET			mSock = INVALID_SOCKET;			//Cliet와 연결되는 소켓
	stOverlappedEx	mRecvOverlappedEx;	//RECV Overlapped I/O작업을 위한 변수
	stOverlappedEx	mSendOverlappedEx;	//SEND Overlapped I/O작업을 위한 변수

	char			mRecvBuf[MAX_SOCKBUF]; //데이터 버퍼

	std::mutex		mSendLock;
	bool			mIsSending = false;
	UINT64			mSendPos = 0;
	char			mSendBuf[MAX_SOCK_SENDBUF]; //데이터 버퍼	
	char			mSendingBuf[MAX_SOCK_SENDBUF]; //데이터 버퍼	
};