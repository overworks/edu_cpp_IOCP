#include <iostream>
#include <utility>
#include <cstring>

#include "UserManager.h"
#include "PacketManager.h"
#include "RedisManager.h"

PacketManager::PacketManager(int maxClient, std::function<void(uint32_t, uint32_t, const char*)> sendPacketFunc)
{
	mRecvFuntionDictionary[(int)PACKET_ID::SYS_USER_CONNECT] = &PacketManager::ProcessUserConnect;
	mRecvFuntionDictionary[(int)PACKET_ID::SYS_USER_DISCONNECT] = &PacketManager::ProcessUserDisConnect;

	mRecvFuntionDictionary[(int)PACKET_ID::LOGIN_REQUEST] = &PacketManager::ProcessLogin;

	mRecvFuntionDictionary[(int)RedisTaskID::RESPONSE_LOGIN] = &PacketManager::ProcessLoginDBResult;

	mUserManager = new UserManager();
	mUserManager->Init(maxClient);

	mRedisMgr = new RedisManager();

	mSendPacketFunc = sendPacketFunc;
}

PacketManager::~PacketManager()
{
	delete mUserManager;
	delete mRedisMgr;
}

bool
PacketManager::Run()
{	
	if (mRedisMgr->Run("127.0.0.1", 6379, 1) == false)
	{
		return false;
	}

	//이 부분을 패킷 처리 부분으로 이동 시킨다.
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
PacketManager::ReceivePacketData(int clientIndex, size_t size, const char* pData)
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

		if (auto task = mRedisMgr->TakeResponseTask(); task != nullptr && task->TaskID != RedisTaskID::INVALID)
		{
			isIdle = false;
			ProcessRecvPacket(task->UserIndex, (uint16_t)task->TaskID, task->DataSize, task->pData);
			task->Release();
		}

		if(isIdle)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}
}

void PacketManager::EnqueuePacketData(uint32_t clientIndex)
{
	std::lock_guard<std::mutex> guard(mLock);
	mInComingPacketUserIndex.push(clientIndex);
}

PacketInfo PacketManager::DequePacketData()
{
	int userIndex = 0;

	{
		std::lock_guard<std::mutex> guard(mLock);
		if (mInComingPacketUserIndex.empty())
		{
			return PacketInfo();
		}

		userIndex = mInComingPacketUserIndex.front();
		mInComingPacketUserIndex.pop();
	}

	auto pUser = mUserManager->GetUserByConnIdx(userIndex);
	auto packetData = pUser->GetPacket();
	packetData.ClientIndex = userIndex;
	return packetData;
}

void
PacketManager::PushSystemPacket(PacketInfo packet)
{
	std::lock_guard<std::mutex> guard(mLock);
	mSystemPacketQueue.push(packet);
}

PacketInfo
PacketManager::DequeSystemPacketData()
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
PacketManager::ProcessRecvPacket(uint32_t clientIndex, uint16_t packetId, uint16_t packetSize, const char* pPacket)
{
	if (auto iter = mRecvFuntionDictionary.find(packetId); iter != mRecvFuntionDictionary.end())
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
	ClearConnectionInfo(clientIndex);
}

void
PacketManager::ProcessLogin(uint32_t clientIndex, uint16_t packetSize, const char* pPacket)
{ 
	if (LOGIN_REQUEST_PACKET_SIZE != packetSize)
	{
		return;
	}

	auto pLoginReqPacket = reinterpret_cast<const LOGIN_REQUEST_PACKET*>(pPacket);

	auto pUserID = pLoginReqPacket->UserID;
	std::cout << "requested user id = " << pUserID << std::endl;

	LOGIN_RESPONSE_PACKET loginResPacket;
	loginResPacket.PacketId = (uint16_t)PACKET_ID::LOGIN_RESPONSE;
	loginResPacket.PacketLength = sizeof(LOGIN_RESPONSE_PACKET);

	if (mUserManager->GetCurrentUserCnt() >= mUserManager->GetMaxUserCnt()) 
	{ 
		//접속자수가 최대수를 차지해서 접속불가
		loginResPacket.Result = (uint16_t)ERROR_CODE::LOGIN_USER_USED_ALL_OBJ;
		mSendPacketFunc(clientIndex, sizeof(LOGIN_RESPONSE_PACKET) , (const char*)&loginResPacket);
		return;
	}

	//여기에서 이미 접속된 유저인지 확인하고, 접속된 유저라면 실패한다.
	if (mUserManager->FindUserIndexByID(pUserID) == -1) 
	{ 
		RedisLoginReq dbReq;
		memcpy(dbReq.UserID, pLoginReqPacket->UserID, (MAX_USER_ID_LEN + 1));
		memcpy(dbReq.UserPW, pLoginReqPacket->UserPW, (MAX_USER_PW_LEN + 1));

		auto task = std::shared_ptr<RedisTask>();
		task->UserIndex = clientIndex;
		task->TaskID = RedisTaskID::REQUEST_LOGIN;
		task->DataSize = sizeof(RedisLoginReq);
		task->pData = new char[task->DataSize];
		memcpy(task->pData, &dbReq, task->DataSize);
		mRedisMgr->PushRequest(task);

		std::cout << "Login To Redis user id = " << pUserID << std::endl;
	}
	else 
	{
		//접속중인 유저여서 실패를 반환한다.
		loginResPacket.Result = (uint16_t)ERROR_CODE::LOGIN_USER_ALREADY;
		mSendPacketFunc(clientIndex, sizeof(LOGIN_RESPONSE_PACKET), (const char*)&loginResPacket);
		return;
	}
}

void
PacketManager::ProcessLoginDBResult(uint32_t clientIndex, uint16_t packetSize, const char* pPacket)
{
	std::cout << "ProcessLoginDBResult. UserIndex: " << clientIndex << std::endl;

	auto pBody = (RedisLoginRes*)pPacket;

	if (pBody->Result == (uint16_t)ERROR_CODE::NONE)
	{
		//로그인 완료로 변경한다
	}

	LOGIN_RESPONSE_PACKET loginResPacket;
	loginResPacket.PacketId = (uint16_t)PACKET_ID::LOGIN_RESPONSE;
	loginResPacket.PacketLength = sizeof(LOGIN_RESPONSE_PACKET);
	loginResPacket.Result = pBody->Result;
	mSendPacketFunc(clientIndex, sizeof(LOGIN_RESPONSE_PACKET), (const char*)&loginResPacket);
}

void
PacketManager::ClearConnectionInfo(int clientIndex)
{
	auto pReqUser = mUserManager->GetUserByConnIdx(clientIndex);
		
	if (pReqUser->GetDomainState() != User::DOMAIN_STATE::NONE) 
	{
		mUserManager->DeleteUserInfo(pReqUser);
	}
}
