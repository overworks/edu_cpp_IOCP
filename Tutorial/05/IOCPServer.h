//��ó: �����ߴ��� ���� '�¶��� ���Ӽ���'����
#pragma once
#pragma comment(lib, "ws2_32")

#include <memory>
#include "ClientInfo.h"
#include "Define.h"
#include <thread>
#include <vector>

class stClientInfo;

typedef std::shared_ptr<stClientInfo> ClientInfoPtr;

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

	//�����Ǿ��ִ� �����带 �ı��Ѵ�.
	void DestroyThread();

	bool SendMsg(UINT32 sessionIndex, UINT32 dataSize, char* pData);
	
protected:
	virtual void OnConnect(UINT32 clientIndex) = 0;
	virtual void OnClose(UINT32 clientIndex) = 0;
	virtual void OnReceive(UINT32 clientIndex, UINT32 size, const char* pData) = 0;

private:
	void CreateClient(UINT32 maxClientCount);

	//WaitingThread Queue���� ����� ��������� ����
	bool CreateWorkerThread();
	
	//accept��û�� ó���ϴ� ������ ����
	bool CreateAccepterThread();
		  		
	void CreateSendThread();

	//������� �ʴ� Ŭ���̾�Ʈ ���� ����ü�� ��ȯ�Ѵ�.
	ClientInfoPtr GetEmptyClientInfo();

	ClientInfoPtr GetClientInfo(UINT32 sessionIndex)				{ return mClientInfos[sessionIndex]; }
	const ClientInfoPtr GetClientInfo(UINT32 sessionIndex) const	{ return mClientInfos[sessionIndex]; }
	

	//������ ������ ���� ��Ų��.
	void CloseSocket(ClientInfoPtr pClientInfo, bool bIsForce = false);


	//Overlapped I/O�۾��� ���� �Ϸ� �뺸�� �޾� �׿� �ش��ϴ� ó���� �ϴ� �Լ�
	void WorkProcess();
	//������� ������ �޴� ������
	void AcceptProcess();
	// ť�� ���ִ� ��Ŷ�� ������ ��ƾ
	void SendProcess();



	//Ŭ���̾�Ʈ ���� ���� ����ü
	std::vector<ClientInfoPtr> mClientInfos;

	//Ŭ���̾�Ʈ�� ������ �ޱ����� ���� ����
	SOCKET		mListenSocket = INVALID_SOCKET;
	
	//���� �Ǿ��ִ� Ŭ���̾�Ʈ ��
	int			mClientCnt = 0;
	
	//�۾� ������ ���� �÷���
	bool		mIsWorkerRun = false;
	//IO Worker ������
	std::vector<std::thread> mIOWorkerThreads;

	//���� ������ ���� �÷���
	bool		mIsAccepterRun = false;
	//Accept ������
	std::thread	mAccepterThread;

	//���� ������ ���� �÷���
	bool		mIsSenderRun = false;
	//Accept ������
	std::thread	mSendThread;

	//CompletionPort��ü �ڵ�
	HANDLE		mIOCPHandle = INVALID_HANDLE_VALUE;	
};