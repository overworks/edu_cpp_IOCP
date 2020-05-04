#pragma once

#include "IOCPServer.h"
#include "Packet.h"

#include <deque>
#include <thread>
#include <mutex>
#include <memory>

typedef std::shared_ptr<PacketData> PacketDataPtr;

class EchoServer : public IOCPServer
{
public:
	EchoServer() = default;
	virtual ~EchoServer() = default;

	void Run(UINT32 maxClient);
	void End();

protected:
	virtual void OnConnect(UINT32 clientIndex_) override;
	virtual void OnClose(UINT32 clientIndex_) override;
	virtual void OnReceive(UINT32 clientIndex, UINT32 size, const char* pData) override;

private:
	void ProcessPacket();
	PacketDataPtr DequePacketData();

private:
	bool mIsRunProcessThread = false;

	std::thread mProcessThread;

	std::mutex mLock;
	std::deque<PacketDataPtr> mPacketDataQueue;
};