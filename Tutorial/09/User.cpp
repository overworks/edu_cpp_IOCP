#include <cstring>
#include "User.h"

User::User(int index)
	: mIndex(index)
{
	
}

void
User::Clear()
{
	mRoomIndex = -1;
	mUserID.clear();
	mIsConfirm = false;
	mCurDomainState = DOMAIN_STATE::NONE;

	mPacketDataBufferWPos = 0;
	mPacketDataBufferRPos = 0;
}

void
User::Login(const std::string& userID)
{
	mUserID = userID;
	mCurDomainState = DOMAIN_STATE::LOGIN;
}

void
User::EnterRoom(int roomIndex)
{
	mRoomIndex = roomIndex;
	mCurDomainState = DOMAIN_STATE::ROOM;
}

void
User::SetPacketData(size_t dataSize, const char* pData)
{
	if ((mPacketDataBufferWPos + dataSize) >= PACKET_DATA_BUFFER_SIZE)
	{
		auto remainDataSize = mPacketDataBufferWPos - mPacketDataBufferRPos;

		if (remainDataSize > 0)
		{
			memcpy(mPakcetDataBuffer, mPakcetDataBuffer + mPacketDataBufferRPos, remainDataSize);
			mPacketDataBufferWPos = remainDataSize;
		}
		else
		{
			mPacketDataBufferWPos = 0;
		}

		mPacketDataBufferRPos = 0;
	}

	memcpy(mPakcetDataBuffer + mPacketDataBufferWPos, pData, dataSize);
	mPacketDataBufferWPos += dataSize;
}

PacketInfo
User::GetPacket()
{
	const int PACKET_SIZE_LENGTH = 2;
	const int PACKET_TYPE_LENGTH = 2;

	auto remainByte = mPacketDataBufferWPos - mPacketDataBufferRPos;

	PacketInfo packetInfo;
	if (remainByte < PACKET_HEADER_LENGTH)
	{
		return packetInfo;
	}

	auto pHeader = (PACKET_HEADER*)&mPakcetDataBuffer[mPacketDataBufferRPos];

	if (pHeader->PacketLength > remainByte)
	{
		return packetInfo;
	}

	packetInfo.PacketId = pHeader->PacketId;
	packetInfo.DataSize = pHeader->PacketLength;
	packetInfo.pDataPtr = &mPakcetDataBuffer[mPacketDataBufferRPos];

	mPacketDataBufferRPos += pHeader->PacketLength;

	return packetInfo;
}