#pragma once

#include "./ServerNetwork/IOCPServer.h"

//TODO redis 연동. hiredis 포함하기

class PacketManager;

class ChatServer : public IOCPServer
{
public:
	ChatServer();
	virtual ~ChatServer();

	void Run(int maxClient);
	void End();

protected:
	virtual void OnConnect(uint32_t clientIndex) override;
	virtual void OnClose(uint32_t clientIndex) override;
	virtual void OnReceive(uint32_t clientIndex, uint32_t size, const char* pData) override;

private:
	PacketManager* mPacketManager;
};