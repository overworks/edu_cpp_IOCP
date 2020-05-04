//��ó: �����ߴ��� ���� '�¶��� ���Ӽ���'����
#pragma once

#include "ClientInfo.h"
#include "Define.h"
#include <thread>
#include <vector>

class IOCPServer
{
public:
	IOCPServer() = default;
	virtual ~IOCPServer();

	//������ �ʱ�ȭ�ϴ� �Լ�
	bool InitSocket();
		
	//������ �ּ������� ���ϰ� �����Ű�� ���� ��û�� �ޱ� ���� ������ ����ϴ� �Լ�
	bool BindAndListen(int nBindPort);

	//���� ��û�� �����ϰ� �޼����� �޾Ƽ� ó���ϴ� �Լ�
	bool StartServer(UINT32 maxClientCount);

	bool SendMsg(UINT32 sessionIndex, UINT32 dataSize, const char* pData);
	
protected:
	//�����Ǿ��ִ� �����带 �ı��Ѵ�.
	void DestroyThread();

	virtual void OnConnect(UINT32 clientIndex) = 0;
	virtual void OnClose(UINT32 clientIndex) = 0;
	virtual void OnReceive(UINT32 clientIndex, UINT32 size, const char* pData) = 0;

	stClientInfo* GetClientInfo(UINT32 sessionIndex) { return &mClientInfos[sessionIndex]; }
	const stClientInfo* GetClientInfo(UINT32 sessionIndex) const { return &mClientInfos[sessionIndex]; }

private:
	void CreateClient(UINT32 maxClientCount);

	//WaitingThread Queue���� ����� ��������� ����
	bool CreateWorkerThread();

	//accept��û�� ó���ϴ� ������ ����
	bool CreateAccepterThread();
	
	//������� �ʴ� Ŭ���̾�Ʈ ���� ����ü�� ��ȯ�Ѵ�.
	stClientInfo* GetEmptyClientInfo();

	//Overlapped I/O�۾��� ���� �Ϸ� �뺸�� �޾� �׿� �ش��ϴ� ó���� �ϴ� �Լ�
	void WorkerThread();

	//������� ������ �޴� ������
	void AccepterThread();
	
	//������ ������ ���� ��Ų��.
	void CloseSocket(stClientInfo* pClientInfo, bool bIsForce = false);


private:
	//Ŭ���̾�Ʈ ���� ���� ����ü
	std::vector<stClientInfo> mClientInfos;

	//Ŭ���̾�Ʈ�� ������ �ޱ����� ���� ����
	SOCKET		mListenSocket = INVALID_SOCKET;
	
	//���� �Ǿ��ִ� Ŭ���̾�Ʈ ��
	int			mClientCnt = 0;
	
	//IO Worker ������
	std::vector<std::thread> mIOWorkerThreads;

	//Accept ������
	std::thread	mAccepterThread;

	//CompletionPort��ü �ڵ�
	HANDLE		mIOCPHandle = INVALID_HANDLE_VALUE;
	
	//�۾� ������ ���� �÷���
	bool		mIsWorkerRun = true;

	//���� ������ ���� �÷���
	bool		mIsAccepterRun = true;
};