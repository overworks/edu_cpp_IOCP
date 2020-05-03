//��ó: �����ߴ��� ���� '�¶��� ���Ӽ���'����
#pragma once

#include "Define.h"
#include <thread>
#include <vector>

class IOCPServer
{
public:
	IOCPServer();
	virtual ~IOCPServer();

	//������ �ʱ�ȭ�ϴ� �Լ�
	bool InitSocket();

	//������ �ּ������� ���ϰ� �����Ű�� ���� ��û�� �ޱ� ���� ������ ����ϴ� �Լ�
	bool BindAndListen(int nBindPort);

	//���� ��û�� �����ϰ� �޼����� �޾Ƽ� ó���ϴ� �Լ�
	bool StartServer(UINT32 maxClientCount);

	//�����Ǿ��ִ� �����带 �ı��Ѵ�.
	void DestroyThread();

protected:
	//WSASend Overlapped I/O�۾��� ��Ų��.
	bool SendMsg(stClientInfo* pClientInfo, char* pMsg, int nLen);

	virtual void OnConnect(UINT32 clientIndex) = 0;
	virtual void OnClose(UINT32 clientIndex) = 0;
	virtual void OnReceive(UINT32 clientIndex, UINT32 size, char* pData) = 0;

	stClientInfo& GetClientInfo(int index)				{ return mClientInfos[index]; }
	const stClientInfo& GetClientInfo(int index) const	{ return mClientInfos[index]; }

private:
	void CreateClient(UINT32 maxClientCount);

	//WaitingThread Queue���� ����� ��������� ����
	bool CreateWorkerThread();

	//accept��û�� ó���ϴ� ������ ����
	bool CreateAccepterThread();

	//������� �ʴ� Ŭ���̾�Ʈ ���� ����ü�� ��ȯ�Ѵ�.
	stClientInfo* GetEmptyClientInfo();

	//CompletionPort��ü�� ���ϰ� CompletionKey��
	//�����Ű�� ������ �Ѵ�.
	bool BindIOCompletionPort(stClientInfo* pClientInfo);
  	
	//WSARecv Overlapped I/O �۾��� ��Ų��.
	bool BindRecv(stClientInfo* pClientInfo);

	//Overlapped I/O�۾��� ���� �Ϸ� �뺸�� �޾� 
	//�׿� �ش��ϴ� ó���� �ϴ� �Լ�
	void WokerThread();

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
	//���� ����
	char		mSocketBuf[1024] = { 0, };
};