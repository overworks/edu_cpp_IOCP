#pragma once
#include <string>
#include <array>

#include "Packet.h"

constexpr uint32_t PACKET_DATA_BUFFER_SIZE = 8096;

class User
{
public:
	enum class DOMAIN_STATE 
	{
		NONE = 0,
		LOGIN = 1,
		ROOM = 2
	};

	User(int index);
	~User();

	void Clear();
	void Login(const std::string& userID);
	void EnterRoom(int roomIndex);
	void LeaveRoom();
		
	void SetDomainState(DOMAIN_STATE value) { mCurDomainState = value; }
	int GetCurrentRoom() const { return mRoomIndex; }
	int GetNetConnIdx() const { return mIndex; }
	const std::string& GetUserId() const { return  mUserID; }
	DOMAIN_STATE GetDomainState() const { return mCurDomainState; }
				
	void SetPacketData(uint32_t dataSize, const char* pData);

	PacketInfo GetPacket();


private:
	int mIndex = -1;
	int mRoomIndex = -1;
	DOMAIN_STATE mCurDomainState = DOMAIN_STATE::NONE;

	std::string mUserID;
	std::string mAuthToken;
	
	uint32_t mPacketDataBufferWPos = 0;
	uint32_t mPacketDataBufferRPos = 0;
	char mPacketDataBuffer[PACKET_DATA_BUFFER_SIZE];
};

