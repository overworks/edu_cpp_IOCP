#include <string.h>
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

	mPakcetDataBufferWPos = 0;
	mPakcetDataBufferRPos = 0;
}

int
User::SetLogin(const std::string& userID)
{
	mCurDomainState = DOMAIN_STATE::LOGIN;
	mUserID = userID;

	return 0;
}

void
User::EnterRoom(int roomIndex)
{
	mRoomIndex = roomIndex;
	mCurDomainState = DOMAIN_STATE::ROOM;
}

void
User::SetPacketData(uint32_t dataSize, const char* pData)
{
	if ((mPakcetDataBufferWPos + dataSize) >= PACKET_DATA_BUFFER_SIZE)
	{
		auto remainDataSize = mPakcetDataBufferWPos - mPakcetDataBufferRPos;

		if (remainDataSize > 0)
		{
			memcpy(&mPakcetDataBuffer[0], &mPakcetDataBuffer[mPakcetDataBufferRPos], remainDataSize);
			mPakcetDataBufferWPos = remainDataSize;
		}
		else
		{
			mPakcetDataBufferWPos = 0;
		}

		mPakcetDataBufferRPos = 0;
	}

	memcpy(&mPakcetDataBuffer[mPakcetDataBufferWPos], pData, dataSize);
	mPakcetDataBufferWPos += dataSize;
}

PacketInfoPtr
User::GetPacket()
{
	auto remainByte = mPakcetDataBufferWPos - mPakcetDataBufferRPos;

	if (remainByte < PACKET_HEADER_LENGTH)
	{
		return nullptr;
	}

	auto pHeader = (PACKET_HEADER*)&mPakcetDataBuffer[mPakcetDataBufferRPos];

	if (pHeader->PacketLength > remainByte)
	{
		return nullptr;
	}

	PacketInfoPtr packetInfo = std::make_shared<PacketInfo>();
	packetInfo->PacketId = pHeader->PacketId;
	packetInfo->DataSize = pHeader->PacketLength;
	packetInfo->pDataPtr = &mPakcetDataBuffer[mPakcetDataBufferRPos];

	mPakcetDataBufferRPos += pHeader->PacketLength;

	return packetInfo;
}