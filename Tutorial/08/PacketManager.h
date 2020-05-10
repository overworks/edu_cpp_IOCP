#pragma once

#include "Packet.h"

#include <unordered_map>
#include <deque>
#include <functional>
#include <thread>
#include <mutex>
#include <memory>

class UserManager;
class RoomManager;
class RedisManager;

class PacketManager {
public:
	PacketManager(int maxClient);
	~PacketManager();

	bool Run();
	void End();
	void ReceivePacketData(UINT32 clientIndex, UINT32 size, const char* pData);
	void PushSystemPacket(const PacketInfoPtr& packet);
		
	std::function<void(UINT32, UINT32, const char*)> SendPacketFunc;


private:
	void ClearConnectionInfo(UINT32 clientIndex_);

	void EnqueuePacketData(UINT32 clientIndex);
	PacketInfoPtr DequeuePacketData();

	PacketInfoPtr DequeueSystemPacketData();

	void ProcessPacket();
	void ProcessRecvPacket(UINT32 clientIndex, UINT16 packetId, UINT16 packetSize, const char* pPacket);
	void ProcessUserConnect(UINT32 clientIndex, UINT16 packetSize, const char* pPacket);
	void ProcessUserDisconnect(UINT32 clientIndex, UINT16 packetSize, const char* pPacket);
	void ProcessLogin(UINT32 clientIndex, UINT16 packetSize, const char* pPacket);
	
private:
	typedef void(PacketManager::* PROCESS_RECV_PACKET_FUNCTION)(UINT32, UINT16, const char*);
	std::unordered_map<int, PROCESS_RECV_PACKET_FUNCTION> mRecvFuntionDictionary;

	UserManager* mUserManager;

	std::function<void(int, const char*)> mSendMQDataFunc;

	bool mIsRunProcessThread = false;
	
	std::thread mProcessThread;
	
	std::mutex mLock;
	
	std::deque<UINT32> mInComingPacketUserIndex;

	std::deque<PacketInfoPtr> mSystemPacketQueue;
};

