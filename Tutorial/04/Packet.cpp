#include "Packet.h"

PacketData::PacketData(UINT sessionIndex, UINT32 dataSize, const char* pData)
{
	Set(sessionIndex, dataSize, pData);
}

PacketData::~PacketData()
{
	Release();
}

void
PacketData::Set(UINT32 sessionIndex, UINT32 dataSize, const char* pData)
{
	Release();

	SessionIndex = sessionIndex;
	DataSize = dataSize;

	pPacketData = new char[dataSize];
	CopyMemory(pPacketData, pData, dataSize);
}

void
PacketData::Release()
{
	if (pPacketData)
	{
		delete[] pPacketData;
		pPacketData = nullptr;
	}
}