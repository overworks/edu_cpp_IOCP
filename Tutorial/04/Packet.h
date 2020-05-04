#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

struct PacketData
{
	UINT32 SessionIndex = 0;
	UINT32 DataSize = 0;
	char* pPacketData = nullptr;

	PacketData() = default;
	PacketData(UINT sessionIndex, UINT32 dataSize, const char* pData);
	~PacketData();
	void Set(UINT32 sessionIndex, UINT32 dataSize, const char* pData);
	void Release();
};