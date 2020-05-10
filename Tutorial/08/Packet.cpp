#include <cstring>
#include "Packet.h"

RawPacketData::RawPacketData(uint32_t clientIndex, uint32_t dataSize, const char* pData)
{
	ClientIndex = clientIndex;
	DataSize = dataSize;
	pPacketData = new char[dataSize];
	memcpy(pPacketData, pData, dataSize);
}

RawPacketData::~RawPacketData()
{
	Release();
}

void
RawPacketData::Release()
{
	if (pPacketData)
	{
		delete[] pPacketData;
		pPacketData = nullptr;
	}
}