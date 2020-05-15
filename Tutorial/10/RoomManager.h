#pragma once

#include <vector>
#include "Packet.h"
#include "ErrorCode.h"

class Room;
class User;

class RoomManager
{
public:
	RoomManager(int beginRoomNumber, size_t maxRoomCount, uint32_t maxRoomUserCount, SendPacketFunc sendPacketFunc);
	RoomManager(const RoomManager&) = delete;
	~RoomManager();

	size_t GetMaxRoomCount() const { return mRoomList.size(); }
		
	ERROR_CODE EnterUser(int roomNumber, User* user);
	ERROR_CODE LeaveUser(User* user);
	Room* GetRoomByNumber(int number);

private:
	std::vector<Room*> mRoomList;
	int mBeginRoomNumber = 0;
	int mEndRoomNumber = 0;
};
