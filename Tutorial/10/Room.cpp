#include "User.h"
#include "Room.h"

Room::Room(int roomNum, uint32_t maxUserCount, SendPacketFunc sendPacketFunc)
	: mRoomNum(roomNum), mMaxUserCount(maxUserCount), mSendPacketFunc(sendPacketFunc)
{

}

ERROR_CODE
Room::EnterUser(User* user)
{
	if (GetCurrentUserCount() >= GetMaxUserCount())
	{
		return ERROR_CODE::ENTER_ROOM_FULL_USER;
	}

	auto& userID = user->GetUserId();
	if (mUserTable.find(userID) != mUserTable.end())
	{
		// TODO 이미 들어있음.
	}

	mUserTable.insert({ userID, user });
	user->EnterRoom(GetRoomNumber());

	return ERROR_CODE::NONE;
}

void
Room::LeaveUser(User* user)
{
	auto iter = mUserTable.find(user->GetUserId());
	if (iter != mUserTable.end())
	{
		iter->second->LeaveRoom();
		mUserTable.erase(iter);
	}
}

void
Room::NotifyChat(int clientIndex, const std::string& userID, const char* pMsg)
{
	ROOM_CHAT_NOTIFY_PACKET roomChatNtfyPkt;
	roomChatNtfyPkt.PacketId = (uint16_t)PACKET_ID::ROOM_CHAT_NOTIFY;
	roomChatNtfyPkt.PacketLength = sizeof(roomChatNtfyPkt);

	memcpy(roomChatNtfyPkt.Msg, pMsg, sizeof(roomChatNtfyPkt.Msg));
	memcpy(roomChatNtfyPkt.UserID, userID.c_str(), sizeof(roomChatNtfyPkt.UserID));
	SendToAllUser(sizeof(roomChatNtfyPkt), (const char*)&roomChatNtfyPkt, clientIndex, false);
}

void
Room::SendToAllUser(size_t dataSize, const char* data, int passUserIndex, bool exceptMe)
{
	for (auto iter : mUserTable)
	{
		auto pUser = iter.second;

		if (pUser == nullptr)
		{
			continue;
		}

		if (exceptMe && pUser->GetNetConnIdx() == passUserIndex)
		{
			continue;
		}

		mSendPacketFunc((uint32_t)pUser->GetNetConnIdx(), (uint32_t)dataSize, data);
	}
}
