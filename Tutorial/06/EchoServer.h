#pragma once

#include "IOCPServer.h"

#include <vector>
#include <deque>
#include <thread>
#include <mutex>
#include <memory>

struct PacketData;

using PacketDataPtr = std::shared_ptr<PacketData>;

class EchoServer : public IOCPServer
{
public:
	EchoServer() = default;
	virtual ~EchoServer() = default;
	
	void Run(UINT32 maxClient);
	void End();

protected:
	virtual void OnConnect(UINT32 clientIndex) override;
	virtual void OnClose(UINT32 clientIndex) override;
	virtual void OnReceive(UINT32 clientIndex, UINT32 size, const char* pData) override;

private:
	void ProcessPacket();
	PacketDataPtr DequeuePacketData();

private:
	bool mIsRunProcessThread = false;

	std::thread mProcessThread;
	std::mutex mLock;
	std::deque<PacketDataPtr> mPacketDataQueue;
};