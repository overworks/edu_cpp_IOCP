#include <iostream>
#include "Packet.h"

PacketData::PacketData(UINT32 sessionIndex, UINT32 dataSize, const char* pData)
{
	Set(sessionIndex, dataSize, pData);
}

PacketData::PacketData(const PacketData& other)
{
	Set(other);
}

PacketData::~PacketData()
{
	Release();
}

void
PacketData::Set(const PacketData& other)
{
	if (&other == this)
	{
		return;
	}

	Set(other.SessionIndex, other.DataSize, other.pPacketData);
}

void
PacketData::Set(UINT32 sessionIndex, UINT32 dataSize, const char* pData)
{
	if (!pData)
	{
		std::cerr << "[ERROR] pData is nullptr" << std::endl;
		return;
	}

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