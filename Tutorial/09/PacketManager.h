#pragma once

#include "Packet.h"

#include <unordered_map>
#include <queue>
#include <functional>
#include <thread>
#include <mutex>
#include <memory>


class UserManager;
class RedisManager;

class PacketManager {
public:
	typedef void(PacketManager::*PROCESS_RECV_PACKET_FUNCTION)(uint32_t, uint16_t, const char*);

	PacketManager(int maxClient, std::function<void(uint32_t, uint32_t, const char*)> sendPacketFunc);
	~PacketManager();

	bool Run();
	void End();
	void ReceivePacketData(int clientIndex, size_t size, const char* pData);
	void PushSystemPacket(PacketInfo packet);
	


private:
	void ClearConnectionInfo(int clientIndex);

	void EnqueuePacketData(uint32_t clientIndex);
	PacketInfo DequePacketData();
	PacketInfo DequeSystemPacketData();

	void ProcessPacket();

	void ProcessRecvPacket(uint32_t clientIndex, uint16_t packetId, uint16_t packetSize, const char* pPacket_);

	void ProcessUserConnect(uint32_t clientIndex, uint16_t packetSize, const char* pPacket_);
	void ProcessUserDisconnect(uint32_t clientIndex, uint16_t packetSize, const char* pPacket_);
	
	void ProcessLogin(uint32_t clientIndex, uint16_t packetSize, const char* pPacket_);
	void ProcessLoginDBResult(uint32_t clientIndex, uint16_t packetSize, const char* pPacket_);
	
	
	
private:
	std::unordered_map<int, PROCESS_RECV_PACKET_FUNCTION> mRecvFuntionDictionary;

	UserManager* mUserManager;
	RedisManager* mRedisMgr;
		
	std::function<void(uint32_t, uint32_t, const char*)> mSendPacketFunc; 
	std::function<void(int, char*)> mSendMQDataFunc;


	bool mIsRunProcessThread = false;
	
	std::thread mProcessThread;
	
	std::mutex mLock;
	
	std::queue<uint32_t> mInComingPacketUserIndex;

	std::queue<PacketInfo> mSystemPacketQueue;
};

