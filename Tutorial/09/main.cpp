#include "ChatServer.h"
#include <string>
#include <iostream>

const UINT16 SERVER_PORT = 11021;
const UINT16 MAX_CLIENT = 3;		//총 접속할수 있는 클라이언트 수
const UINT32 MAX_IO_WORKER_THREAD = 4;  //쓰레드 풀에 넣을 쓰레드 수

int main()
{
	SetConsoleOutputCP(CP_UTF8);
	setvbuf(stdout, nullptr, _IOFBF, 1024);

	ChatServer server;

	//소켓을 초기화
	server.Init(MAX_IO_WORKER_THREAD);

	//소켓과 서버 주소를 연결하고 등록 시킨다.
	server.BindAndListen(SERVER_PORT);

	server.Run(MAX_CLIENT);

	std::cout << u8"아무 키나 누를 때까지 대기합니다" << std::endl;

	std::string inputCmd;
	while (true)
	{
		std::getline(std::cin, inputCmd);

		if (inputCmd == "quit")
		{
			break;
		}
	}

	server.End();
	return 0;
}

