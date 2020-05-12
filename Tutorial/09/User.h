#pragma once

#include <string>
#include "Packet.h"

class User
{
public:
	enum
	{
		PACKET_DATA_BUFFER_SIZE = 8096
	};

	enum class DOMAIN_STATE 
	{
		NONE = 0,
		LOGIN = 1,
		ROOM = 2
	};


	User(int index);
	~User() = default;

	void Clear();
	void Login(const std::string& userID);
	void EnterRoom(int roomIndex);

	int GetCurrentRoom() const { return mRoomIndex; }
	int GetNetConnIdx() const { return mIndex; }
	const std::string& GetUserId() const { return  mUserID; }
	DOMAIN_STATE GetDomainState() const { return mCurDomainState; }
				
	void SetPacketData(size_t dataSize, const char* pData);

	PacketInfo GetPacket();

private:
	int mIndex = -1;
	int mRoomIndex = -1;

	bool mIsConfirm = false;

	std::string mUserID;
	std::string mAuthToken;
	
	DOMAIN_STATE mCurDomainState = DOMAIN_STATE::NONE;		

	uint32_t mPacketDataBufferWPos = 0;
	uint32_t mPacketDataBufferRPos = 0;
	char mPakcetDataBuffer[PACKET_DATA_BUFFER_SIZE];
};

