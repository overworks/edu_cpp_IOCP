#include "EchoServer.h"
#include <string>
#include <iostream>

const UINT16 SERVER_PORT = 11021;
const UINT16 MAX_CLIENT = 100;		//총 접속할수 있는 클라이언트 수

int main()
{
	// utf-8을 사용하기 위한 준비 (https://jacking75.github.io/C++_VC_utf-8_source_code/ 참조)
	SetConsoleOutputCP(CP_UTF8);
	setvbuf(stdout, nullptr, _IOFBF, 1024);

	EchoServer server;

	//소켓을 초기화
	server.InitSocket();

	//소켓과 서버 주소를 연결하고 등록 시킨다.
	server.BindAndListen(SERVER_PORT);

	server.Run(MAX_CLIENT);

	std::cout << u8"아무 키나 누를 때까지 대기합니다" << std::endl;
	while (true)
	{
		std::string inputCmd;
		std::getline(std::cin, inputCmd);

		if (inputCmd == "quit")
		{
			break;
		}
	}

	server.End();
	return 0;
}

