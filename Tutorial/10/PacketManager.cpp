#include <iostream>
#include <utility>
#include <cstring>

#include "User.h"
#include "UserManager.h"
#include "Room.h"
#include "RoomManager.h"
#include "PacketManager.h"
#include "RedisManager.h"

PacketManager::~PacketManager()
{
	Release();
}

void
PacketManager::Release()
{
	if (mRedisMgr)
	{
		delete mRedisMgr;
	}

	if (mUserManager)
	{
		delete mUserManager;
	}

	if (mRoomManager)
	{
		delete mRoomManager;
	}
}

void
PacketManager::Init(uint32_t maxClient, PacketManager::SendPacketFunc sendPacketFunc)
{
	Release();

	mSendPacketFunc = sendPacketFunc;

	mRedisMgr = new RedisManager;// std::make_unique<RedisManager>();
	mUserManager = new UserManager(maxClient);
	mRoomManager = new RoomManager(StartRoomNumber, MaxRoomCount, MaxRoomCount, sendPacketFunc);

	RegisterPacket();
}

void
PacketManager::RegisterPacket()
{
	mRecvFuntionDictionary.clear();

	mRecvFuntionDictionary[(int)PACKET_ID::SYS_USER_CONNECT] = &PacketManager::ProcessUserConnect;
	mRecvFuntionDictionary[(int)PACKET_ID::SYS_USER_DISCONNECT] = &PacketManager::ProcessUserDisconnect;

	mRecvFuntionDictionary[(int)PACKET_ID::LOGIN_REQUEST] = &PacketManager::ProcessLogin;
	mRecvFuntionDictionary[(int)RedisTaskID::RESPONSE_LOGIN] = &PacketManager::ProcessLoginDBResult;

	mRecvFuntionDictionary[(int)PACKET_ID::ROOM_ENTER_REQUEST] = &PacketManager::ProcessEnterRoom;
	mRecvFuntionDictionary[(int)PACKET_ID::ROOM_LEAVE_REQUEST] = &PacketManager::ProcessLeaveRoom;
	mRecvFuntionDictionary[(int)PACKET_ID::ROOM_CHAT_REQUEST] = &PacketManager::ProcessRoomChatMessage;
}

bool
PacketManager::Run()
{	
	if (mRedisMgr->Run("127.0.0.1", 6379, 1) == false)
	{
		return false;
	}

	//이 부분을 패킷 처리 부분으로 이동시킨다.
	mIsRunProcessThread = true;
	mProcessThread = std::thread([this]() { ProcessPacket(); });

	return true;
}

void
PacketManager::End()
{
	mRedisMgr->End();

	mIsRunProcessThread = false;

	if (mProcessThread.joinable())
	{
		mProcessThread.join();
	}
}

void
PacketManager::ClearConnectionInfo(int clientIndex)
{
	auto pReqUser = mUserManager->GetUserByConnIdx(clientIndex);

	if (pReqUser->GetDomainState() == User::DOMAIN_STATE::ROOM)
	{
		mRoomManager->LeaveUser(pReqUser);
	}

	if (pReqUser->GetDomainState() != User::DOMAIN_STATE::NONE)
	{
		mUserManager->DeleteUserInfo(pReqUser);
	}
}

void
PacketManager::ReceivePacketData(int clientIndex, uint32_t dataSize, const char* pData)
{
	auto pUser = mUserManager->GetUserByConnIdx(clientIndex);
	pUser->SetPacketData(dataSize, pData);

	EnqueuePacketData(clientIndex);
}

void
PacketManager::EnqueuePacketData(int clientIndex)
{
	std::lock_guard<std::mutex> guard(mLock);
	mInComingPacketUserIndex.push(clientIndex);
}

PacketInfo
PacketManager::DequePacketData()
{
	int userIndex = 0;

	{
		std::lock_guard<std::mutex> guard(mLock);
		if (mInComingPacketUserIndex.empty())
		{
			return PacketInfo();
		}

		userIndex = static_cast<int>(mInComingPacketUserIndex.front());
		mInComingPacketUserIndex.pop();
	}

	auto pUser = mUserManager->GetUserByConnIdx(userIndex);
	auto packetData = pUser->GetPacket();
	packetData.ClientIndex = userIndex;
	return packetData;
}

void PacketManager::PushSystemPacket(const PacketInfo& packet)
{
	std::lock_guard<std::mutex> guard(mLock);
	mSystemPacketQueue.push(packet);
}

PacketInfo PacketManager::DequeSystemPacketData()
{
	std::lock_guard<std::mutex> guard(mLock);
	if (mSystemPacketQueue.empty())
	{
		return PacketInfo();
	}

	auto packetData = mSystemPacketQueue.front();
	mSystemPacketQueue.pop();

	return packetData;
}


void
PacketManager::ProcessPacket()
{
	while (mIsRunProcessThread)
	{
		bool isIdle = true;

		if (auto packetData = DequePacketData(); packetData.PacketId > (uint16_t)PACKET_ID::SYS_END)
		{
			isIdle = false;
			ProcessRecvPacket(packetData.ClientIndex, packetData.PacketId, packetData.DataSize, packetData.pDataPtr);
		}

		if (auto packetData = DequeSystemPacketData(); packetData.PacketId != 0)
		{
			isIdle = false;
			ProcessRecvPacket(packetData.ClientIndex, packetData.PacketId, packetData.DataSize, packetData.pDataPtr);
		}

		if (auto task = mRedisMgr->PopResponseTask(); task != nullptr && task->TaskID != RedisTaskID::INVALID)
		{
			isIdle = false;
			ProcessRecvPacket(task->UserIndex, static_cast<uint16_t>(task->TaskID), task->DataSize, task->pData);
			//task->Release();
		}

		if(isIdle)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}
}

void
PacketManager::ProcessRecvPacket(uint32_t clientIndex, uint16_t packetId, uint16_t packetSize, const char* pPacket)
{
	auto iter = mRecvFuntionDictionary.find(packetId);
	if (iter != mRecvFuntionDictionary.end())
	{
		(this->*(iter->second))(clientIndex, packetSize, pPacket);
	}

}

void
PacketManager::ProcessUserConnect(uint32_t clientIndex, uint16_t packetSize, const char* pPacket)
{
	std::cout << "[ProcessUserConnect] clientIndex: " << clientIndex << std::endl;
	auto pUser = mUserManager->GetUserByConnIdx(clientIndex);
	pUser->Clear();
}

void
PacketManager::ProcessUserDisconnect(uint32_t clientIndex, uint16_t packetSize, const char* pPacket)
{
	std::cout << "[ProcessUserDisConnect] clientIndex: " << clientIndex << std::endl;
	ClearConnectionInfo((int)clientIndex);
}

void
PacketManager::ProcessLogin(uint32_t clientIndex, uint16_t packetSize, const char* pPacket)
{ 
	if (LOGIN_REQUEST_PACKET_SIZE != packetSize)
	{
		return;
	}

	auto pLoginReqPacket = reinterpret_cast<const LOGIN_REQUEST_PACKET*>(pPacket);

	std::string userID(pLoginReqPacket->UserID);
	std::cout << "requested user id = " << userID << std::endl;

	LOGIN_RESPONSE_PACKET loginResPacket;
	loginResPacket.PacketId = (uint16_t)PACKET_ID::LOGIN_RESPONSE;
	loginResPacket.PacketLength = sizeof(LOGIN_RESPONSE_PACKET);

	if (mUserManager->GetCurrentUserCnt() >= mUserManager->GetMaxUserCnt()) 
	{ 
		//접속자수가 최대수를 차지해서 접속불가
		loginResPacket.Result = static_cast<uint16_t>(ERROR_CODE::LOGIN_USER_USED_ALL_OBJ);
		mSendPacketFunc(clientIndex, sizeof(LOGIN_RESPONSE_PACKET) , reinterpret_cast<const char*>(&loginResPacket));
		return;
	}

	//여기에서 이미 접속된 유저인지 확인하고, 접속된 유저라면 실패한다.
	if (mUserManager->FindUserIndexByID(userID) == -1) 
	{
		RedisLoginReq dbReq;
		memcpy(dbReq.UserID, pLoginReqPacket->UserID, (MAX_USER_ID_LEN + 1));
		memcpy(dbReq.UserPW, pLoginReqPacket->UserPW, (MAX_USER_PW_LEN + 1));

		RedisTaskPtr task = std::make_shared<RedisTask>();
		task->UserIndex = clientIndex;
		task->TaskID = RedisTaskID::REQUEST_LOGIN;
		task->DataSize = sizeof(RedisLoginReq);
		task->pData = new char[task->DataSize];
		memcpy(task->pData, &dbReq, task->DataSize);
		mRedisMgr->PushRequest(task);

		mUserManager->AddUser(userID, static_cast<int>(clientIndex));

		std::cout << "Login To Redis user id = " << userID << std::endl;
	}
	else 
	{
		//접속중인 유저여서 실패를 반환한다.
		loginResPacket.Result = static_cast<uint16_t>(ERROR_CODE::LOGIN_USER_ALREADY);
		mSendPacketFunc(clientIndex, sizeof(LOGIN_RESPONSE_PACKET), reinterpret_cast<const char*>(&loginResPacket));
		return;
	}
}

void PacketManager::ProcessLoginDBResult(uint32_t clientIndex, uint16_t packetSize, const char* pPacket)
{
	std::cout << "ProcessLoginDBResult. UserIndex: " << clientIndex << std::endl;

	auto pBody = reinterpret_cast<const RedisLoginRes*>(pPacket);

	if (pBody->Result == static_cast<uint16_t>(ERROR_CODE::NONE))
	{
		//로그인 완료로 변경한다
	}

	LOGIN_RESPONSE_PACKET loginResPacket;
	loginResPacket.PacketId = static_cast<uint16_t>(PACKET_ID::LOGIN_RESPONSE);
	loginResPacket.PacketLength = sizeof(LOGIN_RESPONSE_PACKET);
	loginResPacket.Result = pBody->Result;
	mSendPacketFunc(clientIndex, sizeof(LOGIN_RESPONSE_PACKET), reinterpret_cast<const char*>(&loginResPacket));
}


void PacketManager::ProcessEnterRoom(uint32_t clientIndex, uint16_t packetSize, const char* pPacket)
{
	//UNREFERENCED_PARAMETER(packetSize);

	auto pReqUser = mUserManager->GetUserByConnIdx(static_cast<int>(clientIndex));

	if (!pReqUser)
	{
		return;
	}
	
	auto pRoomEnterReqPacket = reinterpret_cast<const ROOM_ENTER_REQUEST_PACKET*>(pPacket);
	auto result = mRoomManager->EnterUser(pRoomEnterReqPacket->RoomNumber, pReqUser);

	ROOM_ENTER_RESPONSE_PACKET roomEnterResPacket;
	roomEnterResPacket.PacketId = static_cast<uint16_t>(PACKET_ID::ROOM_ENTER_RESPONSE);
	roomEnterResPacket.PacketLength = sizeof(ROOM_ENTER_RESPONSE_PACKET);			
	roomEnterResPacket.Result = static_cast<uint16_t>(result);

	mSendPacketFunc(clientIndex, sizeof(ROOM_ENTER_RESPONSE_PACKET), reinterpret_cast<const char*>(&roomEnterResPacket));
	std::cout << "Response Packet Sended" << std::endl;
}


void PacketManager::ProcessLeaveRoom(uint32_t clientIndex, uint16_t packetSize, const char* pPacket)
{
	//UNREFERENCED_PARAMETER(packetSize_);
	//UNREFERENCED_PARAMETER(pPacket_);

	ROOM_LEAVE_RESPONSE_PACKET roomLeaveResPacket;
	roomLeaveResPacket.PacketId = static_cast<uint16_t>(PACKET_ID::ROOM_LEAVE_RESPONSE);
	roomLeaveResPacket.PacketLength = sizeof(ROOM_LEAVE_RESPONSE_PACKET);

	auto reqUser = mUserManager->GetUserByConnIdx(clientIndex);
	auto result = mRoomManager->LeaveUser(reqUser);

	//TODO Room안의 UserList객체의 값 확인하기
	roomLeaveResPacket.Result = static_cast<uint16_t>(result);
	mSendPacketFunc(clientIndex, sizeof(ROOM_LEAVE_RESPONSE_PACKET), reinterpret_cast<const char*>(&roomLeaveResPacket));
}


void PacketManager::ProcessRoomChatMessage(uint32_t clientIndex, uint16_t packetSize, const char* pPacket)
{
	//UNREFERENCED_PARAMETER(packetSize_);

	auto pRoomChatReqPacketet = reinterpret_cast<const ROOM_CHAT_REQUEST_PACKET*>(pPacket);

	ROOM_CHAT_RESPONSE_PACKET roomChatResPacket;
	roomChatResPacket.PacketId = static_cast<uint16_t>(PACKET_ID::ROOM_CHAT_RESPONSE);
	roomChatResPacket.PacketLength = sizeof(ROOM_CHAT_RESPONSE_PACKET);
	roomChatResPacket.Result = static_cast<int16_t>(ERROR_CODE::NONE);

	auto reqUser = mUserManager->GetUserByConnIdx(static_cast<int>(clientIndex));
	auto pRoom = mRoomManager->GetRoomByNumber(reqUser->GetCurrentRoom());
	if (pRoom == nullptr)
	{
		roomChatResPacket.Result = static_cast<int16_t>(ERROR_CODE::CHAT_ROOM_INVALID_ROOM_NUMBER);
		mSendPacketFunc(clientIndex, sizeof(ROOM_CHAT_RESPONSE_PACKET), reinterpret_cast<const char*>(&roomChatResPacket));
		return;
	}

	mSendPacketFunc(clientIndex, sizeof(ROOM_CHAT_RESPONSE_PACKET), (char*)&roomChatResPacket);

	pRoom->NotifyChat(clientIndex, reqUser->GetUserId(), pRoomChatReqPacketet->Message);
}
