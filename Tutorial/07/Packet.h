#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

struct PacketData
{
	UINT32 SessionIndex = 0;
	UINT32 DataSize = 0;
	char* pPacketData = nullptr;

	PacketData(UINT32 sessionIndex, UINT32 dataSize, const char* pData);
	~PacketData();

	void Release();
};