#include <iostream>
#include <utility>
#include <cstring>

#include "User.h"
#include "UserManager.h"
#include "PacketManager.h"

PacketManager::PacketManager(int maxClient)
{
	mUserManager = new UserManager(maxClient);

	mRecvFuntionDictionary[(int)PACKET_ID::SYS_USER_CONNECT] = &PacketManager::ProcessUserConnect;
	mRecvFuntionDictionary[(int)PACKET_ID::SYS_USER_DISCONNECT] = &PacketManager::ProcessUserDisconnect;

	mRecvFuntionDictionary[(int)PACKET_ID::LOGIN_REQUEST] = &PacketManager::ProcessLogin;
}

PacketManager::~PacketManager()
{
	delete mUserManager;
}

bool
PacketManager::Run()
{	
	// 이 부분을 패킷 처리 부분으로 이동 시킨다.
	mIsRunProcessThread = true;
	mProcessThread = std::thread([this]() { ProcessPacket(); });

	return true;
}

void
PacketManager::End()
{
	mIsRunProcessThread = false;

	if (mProcessThread.joinable())
	{
		mProcessThread.join();
	}
}

void
PacketManager::ReceivePacketData(UINT32 clientIndex, UINT32 size, const char* pData)
{
	auto pUser = mUserManager->GetUserByConnIdx(clientIndex);
	pUser->SetPacketData(size, pData);

	EnqueuePacketData(clientIndex);
}

void
PacketManager::ProcessPacket()
{
	while (mIsRunProcessThread)
	{
		bool isIdle = true;

		if (auto packetData = DequeuePacketData(); packetData != nullptr && packetData->PacketId > (UINT16)PACKET_ID::SYS_END)
		{
			isIdle = false;
			ProcessRecvPacket(packetData->ClientIndex, packetData->PacketId, packetData->DataSize, packetData->pDataPtr);
		}

		if (auto packetData = DequeueSystemPacketData(); packetData != nullptr && packetData->PacketId != 0)
		{
			isIdle = false;
			ProcessRecvPacket(packetData->ClientIndex, packetData->PacketId, packetData->DataSize, packetData->pDataPtr);
		}
				
		if(isIdle)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}
}

void
PacketManager::EnqueuePacketData(UINT32 clientIndex)
{
	std::lock_guard<std::mutex> guard(mLock);
	mInComingPacketUserIndex.push_back(clientIndex);
}

PacketInfoPtr PacketManager::DequeuePacketData()
{
	UINT32 userIndex = 0;

	{
		std::lock_guard<std::mutex> guard(mLock);
		if (mInComingPacketUserIndex.empty())
		{
			return nullptr;
		}

		userIndex = mInComingPacketUserIndex.front();
		mInComingPacketUserIndex.pop_front();
	}

	auto pUser = mUserManager->GetUserByConnIdx(userIndex);
	auto packetData = pUser->GetPacket();
	if (packetData)
	{
		packetData->ClientIndex = userIndex;
	}
	return packetData;
}

void PacketManager::PushSystemPacket(const PacketInfoPtr& packet)
{
	std::lock_guard<std::mutex> guard(mLock);
	mSystemPacketQueue.push_back(packet);
}

PacketInfoPtr PacketManager::DequeueSystemPacketData()
{
	std::lock_guard<std::mutex> guard(mLock);
	if (mSystemPacketQueue.empty())
	{
		return nullptr;
	}

	auto packetData = mSystemPacketQueue.front();
	mSystemPacketQueue.pop_front();
	
	return packetData;
}

void PacketManager::ProcessRecvPacket(UINT32 clientIndex, UINT16 packetId, UINT16 packetSize, const char* pPacket)
{
	auto iter = mRecvFuntionDictionary.find(packetId);
	if (iter != mRecvFuntionDictionary.end())
	{
		(this->*(iter->second))(clientIndex, packetSize, pPacket);
	}
}


void PacketManager::ProcessUserConnect(UINT32 clientIndex, UINT16 packetSize, const char* pPacket_)
{
	std::cout << "[ProcessUserConnect] clientIndex: " << clientIndex << std::endl;
	
	mUserManager->GetUserByConnIdx(clientIndex)->Clear();
}

void PacketManager::ProcessUserDisconnect(UINT32 clientIndex, UINT16 packetSize, const char* pPacket)
{
	std::cout << "[ProcessUserDisConnect] clientIndex: " << clientIndex << std::endl;

	ClearConnectionInfo(clientIndex);
}

void PacketManager::ProcessLogin(UINT32 clientIndex, UINT16 packetSize, const char* pPacket)
{ 
	if (LOGIN_REQUEST_PACKET_SZIE != packetSize)
	{
		return;
	}

	auto pLoginReqPacket = reinterpret_cast<const LOGIN_REQUEST_PACKET*>(pPacket);

	std::string pUserID(pLoginReqPacket->UserID);
	std::cout << "requested user id = " << pUserID << std::endl;

	LOGIN_RESPONSE_PACKET loginResPacket;
	loginResPacket.PacketId = (UINT16)PACKET_ID::LOGIN_RESPONSE;
	loginResPacket.PacketLength = sizeof(LOGIN_RESPONSE_PACKET);

	if (mUserManager->GetCurrentUserCnt() >= mUserManager->GetMaxUserCnt()) 
	{ 
		// 접속자수가 최대수를 차지해서 접속불가
		loginResPacket.Result = (UINT16)ERROR_CODE::LOGIN_USER_USED_ALL_OBJ;
		SendPacketFunc(clientIndex, sizeof(LOGIN_RESPONSE_PACKET), (const char*)&loginResPacket);
		return;
	}

	// 여기에서 이미 접속된 유저인지 확인하고, 접속된 유저라면 실패한다.
	if (mUserManager->FindUserIndexByID(pUserID) == -1) 
	{ 
		loginResPacket.Result = (UINT16)ERROR_CODE::NONE;
		SendPacketFunc(clientIndex, sizeof(LOGIN_RESPONSE_PACKET), (const char*)&loginResPacket);
	}
	else 
	{
		// 접속중인 유저여서 실패를 반환한다.
		loginResPacket.Result = (UINT16)ERROR_CODE::LOGIN_USER_ALREADY;
		SendPacketFunc(clientIndex, sizeof(LOGIN_RESPONSE_PACKET), (const char*)&loginResPacket);
		return;
	}
}


void PacketManager::ClearConnectionInfo(UINT32 clientIndex) 
{
	auto pReqUser = mUserManager->GetUserByConnIdx(clientIndex);
		
	if (pReqUser->GetDomainState() != User::DOMAIN_STATE::NONE) 
	{
		mUserManager->DeleteUserInfo(pReqUser.get());
	}
}
