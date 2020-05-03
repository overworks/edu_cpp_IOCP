#include "EchoServer.h"
#include <string>
#include <iostream>

const UINT16 SERVER_PORT = 11021;
const UINT16 MAX_CLIENT = 100;		//�� �����Ҽ� �ִ� Ŭ���̾�Ʈ ��

int main()
{
	EchoServer server;

	//������ �ʱ�ȭ
	server.InitSocket();

	//���ϰ� ���� �ּҸ� �����ϰ� ��� ��Ų��.
	server.BindAndListen(SERVER_PORT);

	server.StartServer(MAX_CLIENT);

	std::cout << "�����Ͻ÷��� quit��� ġ�ð� ����Ű�� ��������." << std::endl;
	while (true)
	{
		std::string inputCmd;
		std::getline(std::cin, inputCmd);

		if (inputCmd == "quit")
		{
			break;
		}
	}

	server.DestroyThread();
	return 0;
}

