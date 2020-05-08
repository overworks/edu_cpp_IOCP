#include "Packet.h"

PacketData::PacketData(UINT32 sessionIndex, UINT32 dataSize, const char* pData)
{
	SessionIndex = sessionIndex;
	DataSize = dataSize;
	pPacketData = new char[dataSize];
	CopyMemory(pPacketData, pData, dataSize);
}

PacketData::~PacketData()
{
	Release();
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