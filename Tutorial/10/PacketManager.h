#pragma once

#include "Packet.h"

#include <unordered_map>
#include <queue>
#include <functional>
#include <thread>
#include <mutex>


class UserManager;
class RoomManager;
class RedisManager;

constexpr int StartRoomNumber = 0;
constexpr uint32_t MaxRoomCount = 10;
constexpr uint32_t MaxRoomUserCount = 4;

class PacketManager
{
public:
	using SendPacketFunc = std::function<void(uint32_t, uint32_t, const char*)>;

	PacketManager() = default;
	~PacketManager();

	void Init(uint32_t maxClient, SendPacketFunc sendPacketFunc);
	bool Run();
	void End();
	void ReceivePacketData(int clientIndex, uint32_t dataSize, const char* pData);
	void PushSystemPacket(const PacketInfo& packet);
	
private:
	void Release();
	void RegisterPacket();

	void ClearConnectionInfo(int clientIndex);
	void EnqueuePacketData(int clientIndex);

	PacketInfo DequePacketData();
	PacketInfo DequeSystemPacketData();

	void ProcessPacket();

	void ProcessRecvPacket(uint32_t clientIndex, uint16_t packetId, uint16_t packetSize, const char* pPacket);

	void ProcessUserConnect(uint32_t clientIndex, uint16_t packetSize, const char* pPacket);
	void ProcessUserDisconnect(uint32_t clientIndex, uint16_t packetSize, const char* pPacket);
	
	void ProcessLogin(uint32_t clientIndex, uint16_t packetSize, const char* pPacket);
	void ProcessLoginDBResult(uint32_t clientIndex, uint16_t packetSize, const char* pPacket);
	
	void ProcessEnterRoom(uint32_t clientIndex, uint16_t packetSize, const char* pPacket);
	void ProcessLeaveRoom(uint32_t clientIndex, uint16_t packetSize, const char* pPacket);
	void ProcessRoomChatMessage(uint32_t clientIndex, uint16_t packetSize, const char* pPacket);


private:
	typedef void(PacketManager::* PROCESS_RECV_PACKET_FUNCTION)(uint32_t, uint16_t, const char*);
	std::unordered_map<int, PROCESS_RECV_PACKET_FUNCTION> mRecvFuntionDictionary;

	UserManager* mUserManager;
	RoomManager* mRoomManager;	
	RedisManager* mRedisMgr;
		
	SendPacketFunc mSendPacketFunc; 
	std::function<void(int, char*)> mSendMQDataFunc;

	bool mIsRunProcessThread = false;
	
	std::thread mProcessThread;
	
	std::mutex mLock;
	
	std::queue<uint32_t> mInComingPacketUserIndex;

	std::queue<PacketInfo> mSystemPacketQueue;
};

