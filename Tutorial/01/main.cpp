#include <iostream>
#include "IOCompletionPort.h"

const UINT16 SERVER_PORT = 11021;
const UINT16 MAX_CLIENT = 100;		//총 접속할수 있는 클라이언트 수

int main()
{
	IOCompletionPort ioCompletionPort;

	//소켓을 초기화
	ioCompletionPort.InitSocket();

	//소켓과 서버 주소를 연결하고 등록 시킨다.
	ioCompletionPort.BindAndListen(SERVER_PORT);

	ioCompletionPort.StartServer(MAX_CLIENT);

	std::cout << "아무 키나 누를 때까지 대기합니다" << std::endl;
	getchar();

	ioCompletionPort.DestroyThread();
	return 0;
}

