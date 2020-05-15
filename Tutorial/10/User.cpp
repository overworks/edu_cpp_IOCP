#include "User.h"

User::User(int index)
	: mIndex(index)
{

}

User::~User()
{

}

void
User::Clear()
{
	mRoomIndex = -1;
	mUserID.clear();
	mAuthToken.clear();
	mCurDomainState = DOMAIN_STATE::NONE;

	mPacketDataBufferWPos = 0;
	mPacketDataBufferRPos = 0;
}

void
User::Login(const std::string& userID)
{
	mCurDomainState = DOMAIN_STATE::LOGIN;
	mUserID = userID;
}

void
User::EnterRoom(int roomIndex)
{
	mCurDomainState = DOMAIN_STATE::ROOM; 
	mRoomIndex = roomIndex;
}

void
User::LeaveRoom()
{
	if (mCurDomainState == DOMAIN_STATE::ROOM)
	{
		mCurDomainState = DOMAIN_STATE::LOGIN;
		mRoomIndex = -1;
	}
}

void
User::SetPacketData(uint32_t dataSize, const char* pData)
{
	if ((mPacketDataBufferWPos + dataSize) >= PACKET_DATA_BUFFER_SIZE)
	{
		auto remainDataSize = mPacketDataBufferWPos - mPacketDataBufferRPos;

		if (remainDataSize > 0)
		{
			memcpy(mPacketDataBuffer, mPacketDataBuffer + mPacketDataBufferRPos, remainDataSize);
			mPacketDataBufferWPos = remainDataSize;
		}
		else
		{
			mPacketDataBufferWPos = 0;
		}

		mPacketDataBufferRPos = 0;
	}

	memcpy(mPacketDataBuffer + mPacketDataBufferWPos, pData, dataSize);
	mPacketDataBufferWPos += dataSize;
}

PacketInfo
User::GetPacket()
{
	auto remainByte = mPacketDataBufferWPos - mPacketDataBufferRPos;

	if (remainByte < PACKET_HEADER_LENGTH)
	{
		return PacketInfo();
	}

	auto pHeader = (PACKET_HEADER*)&mPacketDataBuffer[mPacketDataBufferRPos];

	if (pHeader->PacketLength > remainByte)
	{
		return PacketInfo();
	}

	PacketInfo packetInfo;
	packetInfo.PacketId = pHeader->PacketId;
	packetInfo.DataSize = pHeader->PacketLength;
	packetInfo.pDataPtr = &mPacketDataBuffer[mPacketDataBufferRPos];

	mPacketDataBufferRPos += pHeader->PacketLength;

	return packetInfo;
}
