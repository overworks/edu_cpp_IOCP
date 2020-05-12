#pragma once

#include "./ServerNetwork/IOCPServer.h"
#include "PacketManager.h"
#include "Packet.h"

#include <vector>
#include <deque>
#include <memory>
#include <thread>
#include <mutex>

//TODO redis 연동. hiredis 포함하기

class ChatServer : public IOCPServer
{
public:
	ChatServer() = default;
	virtual ~ChatServer() = default;
	
	void Run(uint32_t maxClient);
	void End();

protected:
	virtual void OnConnect(uint32_t clientIndex) override;
	virtual void OnClose(uint32_t clientIndex) override;
	virtual void OnReceive(uint32_t clientIndex, uint32_t size, const char* pData) override;

private:	
	std::unique_ptr<PacketManager> m_pPacketManager;
};