#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

struct PacketData
{
	UINT32 SessionIndex = 0;
	UINT32 DataSize = 0;
	char* pPacketData = nullptr;

	PacketData() = default;
	PacketData(UINT32 sessionIndex, UINT32 dataSize, const char* pData);
	PacketData(const PacketData& other);
	~PacketData();

	void Set(const PacketData& value);
	void Set(UINT32 sessionIndex, UINT32 dataSize, const char* pData);
	void Release();
};