#include <iostream>
#include <algorithm>
#include "../thirdparty/CRedisConn.h"
#include "RedisManager.h"

RedisManager::RedisManager()
{
	mConn = new RedisCpp::CRedisConn();
}

RedisManager::~RedisManager()
{
	delete mConn;
}

bool
RedisManager::Connect(const std::string& ip, uint16_t port)
{
	if (!mConn->connect(ip, port))
	{
		std::cout << "connect error " << mConn->getErrorStr() << std::endl;
		return false;
	}
	
	std::cout << "connect success !!!" << std::endl;
	return true;
}

bool
RedisManager::Run(const std::string& ip, uint16_t port, size_t threadCount)
{
	if (Connect(ip, port) == false)
	{
		std::cerr << u8"Redis 접속 실패" << std::endl;
		return false;
	}

	mIsTaskRun = true;

	for (size_t i = 0; i < threadCount; i++)
	{
		mTaskThreads.emplace_back([this]() { TaskProcessThread(); });
	}

	std::cout << u8"Redis 동작 중..." << std::endl;
	return true;
}

void
RedisManager::End()
{
	JoinThreads();
}

void
RedisManager::JoinThreads()
{
	mIsTaskRun = false;
	std::for_each(mTaskThreads.begin(), mTaskThreads.end(), [](std::thread& th) { if (th.joinable()) th.join(); });
}

void
RedisManager::TaskProcessThread()
{
	std::cout << u8"Redis 스레드 시작..." << std::endl;

	while (mIsTaskRun)
	{
		bool isIdle = true;

		if (auto task = TakeRequestTask(); task != nullptr && task->TaskID != RedisTaskID::INVALID)
		{
			isIdle = false;

			if (task->TaskID == RedisTaskID::REQUEST_LOGIN)
			{
				auto pRequest = reinterpret_cast<RedisLoginReq*>(task->pData);

				RedisLoginRes bodyData;
				bodyData.Result = (UINT16)ERROR_CODE::LOGIN_USER_INVALID_PW;

				std::string value;
				if (mConn->get(pRequest->UserID, value))
				{
					bodyData.Result = (UINT16)ERROR_CODE::NONE;

					if (value.compare(pRequest->UserPW) == 0)
					{
						bodyData.Result = (UINT16)ERROR_CODE::NONE;
					}
				}

				auto resTask = std::make_shared<RedisTask>();
				resTask->UserIndex = task->UserIndex;
				resTask->TaskID = RedisTaskID::RESPONSE_LOGIN;
				resTask->DataSize = sizeof(RedisLoginRes);
				resTask->pData = new char[resTask->DataSize];
				memcpy(resTask->pData, (char*)&bodyData, resTask->DataSize);

				PushResponse(resTask);
			}

			task->Release();
		}


		if (isIdle)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}

	std::cout << u8"Redis 스레드 종료" << std::endl;
}

void
RedisManager::PushRequest(RedisTaskPtr task)
{
	std::lock_guard<std::mutex> guard(mReqLock);
	mRequestTask.push(task);
}

RedisTaskPtr
RedisManager::TakeResponseTask()
{
	std::lock_guard<std::mutex> guard(mResLock);

	if (mResponseTask.empty())
	{
		return nullptr;
	}

	auto task = mResponseTask.front();
	mResponseTask.pop();

	return task;
}

void
RedisManager::PushResponse(RedisTaskPtr task)
{
	std::lock_guard<std::mutex> guard(mResLock);
	mResponseTask.push(task);
}

RedisTaskPtr
RedisManager::TakeRequestTask()
{
	std::lock_guard<std::mutex> guard(mReqLock);

	if (mRequestTask.empty())
	{
		return nullptr;
	}

	auto task = mRequestTask.front();
	mRequestTask.pop();

	return task;
}