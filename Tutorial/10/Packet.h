#pragma once

#include <cstdint>
#include <cstring>
#include <functional>

using SendPacketFunc = std::function<void(uint32_t, uint32_t, const char*)>;

struct RawPacketData
{
	uint32_t ClientIndex = 0;
	uint32_t DataSize = 0;
	char* pPacketData = nullptr;

	void Set(const RawPacketData& value)
	{
		ClientIndex = value.ClientIndex;
		DataSize = value.DataSize;

		pPacketData = new char[value.DataSize];
		memcpy(pPacketData, value.pPacketData, value.DataSize);
	}

	void Set(uint32_t clientIndex, uint32_t dataSize, const char* pData)
	{
		ClientIndex = clientIndex;
		DataSize = dataSize;

		pPacketData = new char[dataSize];
		memcpy(pPacketData, pData, dataSize);
	}

	void Release()
	{
		if (pPacketData)
		{
			delete pPacketData;
			pPacketData = nullptr;
		}
	}
};


struct PacketInfo
{
	uint32_t ClientIndex = 0;
	uint16_t PacketId = 0;
	uint16_t DataSize = 0;
	char* pDataPtr = nullptr;
};


enum class PACKET_ID : uint16_t
{
	//SYSTEM
	SYS_USER_CONNECT = 11,
	SYS_USER_DISCONNECT = 12,
	SYS_END = 30,

	//DB
	DB_END = 199,

	//Client
	LOGIN_REQUEST = 201,
	LOGIN_RESPONSE = 202,

	ROOM_ENTER_REQUEST = 206,
	ROOM_ENTER_RESPONSE = 207,

	ROOM_LEAVE_REQUEST = 215,
	ROOM_LEAVE_RESPONSE = 216,

	ROOM_CHAT_REQUEST = 221,
	ROOM_CHAT_RESPONSE = 222,
	ROOM_CHAT_NOTIFY = 223,
};


#pragma pack(push,1)
struct PACKET_HEADER
{
	uint16_t PacketLength;
	uint16_t PacketId;
	uint8_t Type; //���࿩�� ��ȣȭ���� �� �Ӽ��� �˾Ƴ��� ��
};

constexpr size_t PACKET_HEADER_LENGTH = sizeof(PACKET_HEADER);

//- �α��� ��û
constexpr int MAX_USER_ID_LEN = 32;
constexpr int MAX_USER_PW_LEN = 32;

struct LOGIN_REQUEST_PACKET : public PACKET_HEADER
{
	char UserID[MAX_USER_ID_LEN + 1];
	char UserPW[MAX_USER_PW_LEN + 1];
};
constexpr size_t LOGIN_REQUEST_PACKET_SIZE = sizeof(LOGIN_REQUEST_PACKET);


struct LOGIN_RESPONSE_PACKET : public PACKET_HEADER
{
	uint16_t Result;
};



//- �뿡 ���� ��û
//const int MAX_ROOM_TITLE_SIZE = 32;
struct ROOM_ENTER_REQUEST_PACKET : public PACKET_HEADER
{
	int32_t RoomNumber;
};

struct ROOM_ENTER_RESPONSE_PACKET : public PACKET_HEADER
{
	int16_t Result;
	//char RivalUserID[MAX_USER_ID_LEN + 1] = { 0, };
};


//- �� ������ ��û
struct ROOM_LEAVE_REQUEST_PACKET : public PACKET_HEADER
{
};

struct ROOM_LEAVE_RESPONSE_PACKET : public PACKET_HEADER
{
	int16_t Result;
};



// �� ä��
const int MAX_CHAT_MSG_SIZE = 256;
struct ROOM_CHAT_REQUEST_PACKET : public PACKET_HEADER
{
	char Message[MAX_CHAT_MSG_SIZE + 1] = { 0, };
};

struct ROOM_CHAT_RESPONSE_PACKET : public PACKET_HEADER
{
	int16_t Result;
};

struct ROOM_CHAT_NOTIFY_PACKET : public PACKET_HEADER
{
	char UserID[MAX_USER_ID_LEN + 1] = { 0, };
	char Msg[MAX_CHAT_MSG_SIZE + 1] = { 0, };
};
#pragma pack(pop) //���� ������ ��ŷ������ �����

