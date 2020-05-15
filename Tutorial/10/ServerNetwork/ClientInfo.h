#pragma once

#include "Define.h"
#include <mutex>
#include <queue>
#include <chrono>


//Ŭ���̾�Ʈ ������ ������� ����ü
class stClientInfo
{
public:
	stClientInfo(int index, HANDLE hIOCP);

	int GetIndex() const					{ return mIndex; }
	bool IsConnected() const				{ return mIsConnect; }
	SOCKET GetSock() const					{ return mSocket; }
	std::chrono::seconds GetLatestClosedTimeSec() const { return mLatestClosedTimeSec; }

	char* RecvBuffer()						{ return mRecvBuf; }
	const char* RecvBuffer() const			{ return mRecvBuf; }

	bool OnConnect(HANDLE hIOCP, SOCKET socket);
	void Close(bool bIsForce = false);
	void Clear();
	bool PostAccept(SOCKET listenSock, std::chrono::seconds curTimeSec);
	bool AcceptCompletion();
	bool BindIOCompletionPort(HANDLE hIOCP);
	bool BindRecv();

	// 1���� �����忡���� ȣ���ؾ� �Ѵ�!
	bool SendMsg(uint32_t dataSize, const char* pMsg);
	void SendCompleted(uint32_t dataSize);

private:
	bool SendIO();
	bool SetSocketOption();


private:
	int				mIndex = -1;
	bool			mIsConnect = false;
	HANDLE			mIOCPHandle = INVALID_HANDLE_VALUE;

	SOCKET			mSocket = INVALID_SOCKET;			//Cliet�� ����Ǵ� ����

	stOverlappedEx	mAcceptContext;
	char			mAcceptBuf[64];

	stOverlappedEx	mRecvOverlappedEx;	//RECV Overlapped I/O�۾��� ���� ����	
	char			mRecvBuf[MAX_SOCK_RECVBUF]; //������ ����

	std::mutex		mSendLock;
	std::queue<stOverlappedEx*> mSendDataQueue;

	std::chrono::seconds	mLatestClosedTimeSec;	
};