#include "User.h"
#include "Room.h"
#include "RoomManager.h"

RoomManager::RoomManager(int beginRoomNumber, size_t maxRoomCount, uint32_t maxRoomUserCount, SendPacketFunc sendPacketFunc)
	: mBeginRoomNumber(beginRoomNumber)
{
	mEndRoomNumber = beginRoomNumber + maxRoomCount;

	mRoomList.reserve(maxRoomCount);
	for (size_t i = 0; i < maxRoomCount; ++i)
	{
		Room* pRoom = new Room(beginRoomNumber + i, maxRoomUserCount, sendPacketFunc);
		mRoomList.push_back(pRoom);
	}
}

RoomManager::~RoomManager()
{
	for (auto pRoom : mRoomList)
	{
		delete pRoom;
	}
	mRoomList.clear();
}

ERROR_CODE
RoomManager::EnterUser(int roomNumber, User* user)
{
	Room* pRoom = GetRoomByNumber(roomNumber);
	if (pRoom == nullptr)
	{
		return ERROR_CODE::ROOM_INVALID_INDEX;
	}

	return pRoom->EnterUser(user);
}

ERROR_CODE
RoomManager::LeaveUser(User* user)
{
	int roomNumber = user->GetCurrentRoom();
	Room* pRoom = GetRoomByNumber(roomNumber);
	if (pRoom == nullptr)
	{
		return ERROR_CODE::ROOM_INVALID_INDEX;
	}

	pRoom->LeaveUser(user);
	return ERROR_CODE::NONE;
}

Room*
RoomManager::GetRoomByNumber(int number)
{
	if (number < mBeginRoomNumber || number >= mBeginRoomNumber + (int)GetMaxRoomCount())
	{
		return nullptr;
	}

	int index = (number - mBeginRoomNumber);
	return mRoomList[index];
}
