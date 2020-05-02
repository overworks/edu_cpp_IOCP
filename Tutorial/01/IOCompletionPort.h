//��ó: �����ߴ��� ���� '�¶��� ���Ӽ���'����
#pragma once
#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <Ws2tcpip.h>

#include <thread>
#include <vector>

#define MAX_SOCKBUF 1024	//��Ŷ ũ��
#define MAX_WORKERTHREAD 4  //������ Ǯ�� ���� ������ ��

enum class IOOperation
{
	RECV,
	SEND
};

//WSAOVERLAPPED ����ü�� Ȯ�� ���Ѽ� �ʿ��� ������ �� �־���.
struct stOverlappedEx
{
	WSAOVERLAPPED	m_wsaOverlapped;		//Overlapped I/O����ü
	SOCKET			m_socketClient;			//Ŭ���̾�Ʈ ����
	WSABUF			m_wsaBuf;				//Overlapped I/O�۾� ����
	char			m_szBuf[ MAX_SOCKBUF ]; //������ ����
	IOOperation		m_eOperation;			//�۾� ���� ����
};	
	
//Ŭ���̾�Ʈ ������ ������� ����ü
struct stClientInfo
{
	SOCKET			m_socketClient;			//Cliet�� ����Ǵ� ����
	stOverlappedEx	m_stRecvOverlappedEx;	//RECV Overlapped I/O�۾��� ���� ����
	stOverlappedEx	m_stSendOverlappedEx;	//SEND Overlapped I/O�۾��� ���� ����
	
	stClientInfo();
};


class IOCompletionPort
{
public:
	IOCompletionPort(void);
	~IOCompletionPort(void);

	//������ �ʱ�ȭ�ϴ� �Լ�
	bool InitSocket();

	
	//------������ �Լ�-------//
	//������ �ּ������� ���ϰ� �����Ű�� ���� ��û�� �ޱ� ���� 
	//������ ����ϴ� �Լ�
	bool BindAndListen(int nBindPort);

	//���� ��û�� �����ϰ� �޼����� �޾Ƽ� ó���ϴ� �Լ�
	bool StartServer(size_t maxClientCount);

	//�����Ǿ��ִ� �����带 �ı��Ѵ�.
	void DestroyThread();


private:
	void CreateClient(UINT32 maxClientCount);

	//WaitingThread Queue���� ����� ��������� ����
	bool CreateWokerThread();
	
	//accept��û�� ó���ϴ� ������ ����
	bool CreateAccepterThread();

	//������� �ʴ� Ŭ���̾�Ʈ ���� ����ü�� ��ȯ�Ѵ�.
	stClientInfo* GetEmptyClientInfo();

	//CompletionPort��ü�� ���ϰ� CompletionKey��
	//�����Ű�� ������ �Ѵ�.
	bool BindIOCompletionPort(stClientInfo* pClientInfo);
  	
	//WSARecv Overlapped I/O �۾��� ��Ų��.
	bool BindRecv(stClientInfo* pClientInfo);

	//WSASend Overlapped I/O �۾��� ��Ų��.
	bool SendMsg(stClientInfo* pClientInfo, char* pMsg, int nLen);

	//Overlapped I/O�۾��� ���� �Ϸ� �뺸�� �޾� 
	//�׿� �ش��ϴ� ó���� �ϴ� �Լ�
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

	//���� ����
	char		mSocketBuf[1024] = { 0, };
};