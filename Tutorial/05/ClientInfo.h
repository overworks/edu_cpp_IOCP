#pragma once

#include "Define.h"
#include <stdio.h>
#include <mutex>

//Ŭ���̾�Ʈ ������ ������� ����ü
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

	// 1���� �����忡���� ȣ���ؾ� �Ѵ�!
	bool SendMsg(UINT32 dataSize, const char* pMsg);
	bool SendIO();
	void SendCompleted(UINT32 dataSize_);

private:
	UINT32			mIndex = 0;
	SOCKET			mSock = INVALID_SOCKET;			//Cliet�� ����Ǵ� ����
	stOverlappedEx	mRecvOverlappedEx;	//RECV Overlapped I/O�۾��� ���� ����
	stOverlappedEx	mSendOverlappedEx;	//SEND Overlapped I/O�۾��� ���� ����

	char			mRecvBuf[MAX_SOCKBUF]; //������ ����

	std::mutex		mSendLock;
	bool			mIsSending = false;
	UINT64			mSendPos = 0;
	char			mSendBuf[MAX_SOCK_SENDBUF]; //������ ����	
	char			mSendingBuf[MAX_SOCK_SENDBUF]; //������ ����	
};