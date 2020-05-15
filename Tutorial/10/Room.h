#pragma once

#include <unordered_map>
#include "ErrorCode.h"
#include "Packet.h"

class User;

class Room 
{
public:
	Room(int roomNum, uint32_t maxUserCount, SendPacketFunc sendPacketFunc);
	Room(const Room&) = delete;
	~Room() = default;

	uint32_t GetMaxUserCount() const		{ return mMaxUserCount; }
	uint32_t GetCurrentUserCount() const	{ return static_cast<uint32_t>(mUserTable.size()); }
	int GetRoomNumber() const				{ return mRoomNum; }

	ERROR_CODE EnterUser(User* user);
	void LeaveUser(User* leaveUser);
						
	void NotifyChat(int clientIndex, const std::string& userID, const char* pMsg);

private:
	void SendToAllUser(size_t dataSize, const char* data, int passUserIndex, bool exceptMe);

private:
	uint32_t mMaxUserCount = 0;
	SendPacketFunc mSendPacketFunc;

	int mRoomNum = -1;
	std::unordered_map<std::string, User*> mUserTable;
	
};
