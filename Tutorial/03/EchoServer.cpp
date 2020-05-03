#include "EchoServer.h"

void
EchoServer::OnConnect(UINT32 clientIndex)
{
	printf("[OnConnect] 클라이언트: Index(%d)\n", clientIndex);
}

void
EchoServer::OnClose(UINT32 clientIndex)
{
	printf("[OnClose] 클라이언트: Index(%d)\n", clientIndex);
}

void
EchoServer::OnReceive(UINT32 clientIndex, UINT32 size, char* pData)
{
	stClientInfo& clientInfo = GetClientInfo(clientIndex);
	SendMsg(&clientInfo, pData, size);
}