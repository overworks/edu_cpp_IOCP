#pragma once

#include "IOCPServer.h"

class EchoServer : public IOCPServer
{
	virtual void OnConnect(UINT32 clientIndex) override;
	virtual void OnClose(UINT32 clientIndex) override;
	virtual void OnReceive(UINT32 clientIndex, UINT32 size, char* pData) override;
};