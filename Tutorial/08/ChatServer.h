#pragma once

#include <memory>

#include "./ServerNetwork/IOCPServer.h"
#include "PacketManager.h"

class PacketManager;

//TODO redis 연동. hiredis 포함하기

class ChatServer : public IOCPServer
{
public:
	ChatServer() = default;
	virtual ~ChatServer() = default;
	
	void Run(UINT32 maxClient);
	void End();

protected:
	virtual void OnConnect(UINT32 clientIndex) override;
	virtual void OnClose(UINT32 clientIndex) override;
	virtual void OnReceive(UINT32 clientIndex, UINT32 size, const char* pData) override;

private:	
	std::unique_ptr<PacketManager> m_pPacketManager;
};