#pragma once
#include <cstdint>
#include <cstring>
#include <string>
#include <memory>

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

public:
	User(int index);
	~User() = default;
	
	void Clear();

	int SetLogin(const std::string& userID);
	void EnterRoom(int roomIndex);
		
	void SetDomainState(DOMAIN_STATE value_) { mCurDomainState = value_; }

	int	GetNetConnIdx() const { return mIndex; }
	int GetCurrentRoom() const { return mRoomIndex; }
	DOMAIN_STATE GetDomainState() const { return mCurDomainState; }
	const std::string& GetUserId() const { return  mUserID; }
				
	void SetPacketData(uint32_t dataSize, const char* pData);
	PacketInfoPtr GetPacket();

private:
	int		mIndex = -1;
	int		mRoomIndex = -1;

	std::string mUserID;
	bool mIsConfirm = false;
	std::string mAuthToken;
	
	DOMAIN_STATE mCurDomainState = DOMAIN_STATE::NONE;		

	uint32_t mPakcetDataBufferWPos = 0;
	uint32_t mPakcetDataBufferRPos = 0;
	char mPakcetDataBuffer[PACKET_DATA_BUFFER_SIZE];
};

