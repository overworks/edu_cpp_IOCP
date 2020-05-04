#pragma once

#include "Define.h"
#include <stdio.h>

//Ŭ���̾�Ʈ ������ ������� ����ü
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

	// 1���� �����忡���� ȣ���ؾ� �Ѵ�!
	bool SendMsg(UINT32 dataSize, const char* pMsg) const;
	void SendCompleted(UINT32 dataSize);

private:
	INT32			mIndex;
	SOCKET			mSock;				//Cliet�� ����Ǵ� ����
	stOverlappedEx	mRecvOverlappedEx;	//RECV Overlapped I/O�۾��� ���� ����
	stOverlappedEx	mSendOverlappedEx;
	
	char			mRecvBuf[MAX_SOCKBUF]; //������ ����
	mutable char	mSendBuf[MAX_SOCKBUF];

	//std::queue<stOverlappedEx*> mSendDataqueue;
};