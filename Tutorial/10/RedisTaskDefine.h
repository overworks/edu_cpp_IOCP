#pragma once

#include <cstdint>
#include "ErrorCode.h"

enum class RedisTaskID : uint16_t
{
	INVALID = 0,

	REQUEST_LOGIN = 1001,
	RESPONSE_LOGIN = 1002,
};



struct RedisTask
{
	uint32_t UserIndex = 0;
	RedisTaskID TaskID = RedisTaskID::INVALID;
	uint16_t DataSize = 0;
	char* pData = nullptr;	

	~RedisTask()
	{
		Release();
	}

	void Release()
	{
		if (pData)
		{
			delete[] pData;
			pData = nullptr;
		}
	}
};




#pragma pack(push,1)

struct RedisLoginReq
{
	char UserID[MAX_USER_ID_LEN + 1];
	char UserPW[MAX_USER_PW_LEN + 1];
};

struct RedisLoginRes
{
	uint16_t Result = (uint16_t)ERROR_CODE::NONE;
};

#pragma pack(pop) //위에 설정된 패킹설정이 사라짐