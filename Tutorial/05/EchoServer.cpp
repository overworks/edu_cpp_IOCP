#include "EchoServer.h"
#include "Packet.h"

void
EchoServer::Run(UINT32 maxClient)
{
	mIsRunProcessThread = true;
	mProcessThread = std::thread([this]() { ProcessPacket(); });

	StartServer(maxClient);
}

void
EchoServer::End()
{
	mIsRunProcessThread = false;

	if (mProcessThread.joinable())
	{
		mProcessThread.join();
	}

	DestroyThread();
}

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
EchoServer::OnReceive(UINT32 clientIndex, UINT32 size, const char* pData)
{
	printf("[OnReceive] 클라이언트: Index(%d), dataSize(%d)\n", clientIndex, size);

	std::lock_guard<std::mutex> guard(mLock);
	mPacketDataQueue.push_back(std::make_shared<PacketData>(clientIndex, size, pData));
}

void
EchoServer::ProcessPacket()
{
	while (mIsRunProcessThread)
	{
		auto packetData = DequePacketData();
		if (packetData != nullptr)
		{
			SendMsg(packetData->SessionIndex, packetData->DataSize, packetData->pPacketData);
		}
		else
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}
}

PacketDataPtr
EchoServer::DequePacketData()
{
	std::lock_guard<std::mutex> guard(mLock);
	if (mPacketDataQueue.empty())
	{
		return nullptr;
	}

	auto packetData = mPacketDataQueue.front();
	mPacketDataQueue.pop_front();

	return packetData;
}