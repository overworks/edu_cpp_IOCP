#include <iostream>
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
RedisManager::Run(const std::string& host, uint16_t port, size_t threadCount)
{
	if (!Connect(host, port))
	{
		std::cout << u8"Redis 접속 실패" << std::endl;
		return false;
	}

	mIsTaskRun = true;

	for (uint32_t i = 0; i < threadCount; i++)
	{
		mTaskThreads.emplace_back([this]() { TaskProcessThread(); });
	}

	std::cout << u8"Redis 동작 중..." << std::endl;
	return true;
}

void
RedisManager::End()
{
	mIsTaskRun = false;

	for (auto& thread : mTaskThreads)
	{
		if (thread.joinable())
		{
			thread.join();
		}
	}
}

void
RedisManager::PushRequest(const RedisTaskPtr& task)
{
	std::lock_guard<std::mutex> guard(mReqLock);
	mRequestTask.push(task);
}

RedisTaskPtr
RedisManager::PopResponseTask()
{
	std::lock_guard<std::mutex> guard(mResLock);

	if (mResponseTask.empty())
	{
		return nullptr;
	}

	auto& task = mResponseTask.front();
	mResponseTask.pop();

	return task;
}

bool
RedisManager::Connect(const std::string& host, uint16_t port)
{
	if (mConn->connect(host, port) == false)
	{
		std::cout << "connect error " << mConn->getErrorStr() << std::endl;
		return false;
	}
	else
	{
		std::cout << "connect success !!!" << std::endl;
	}

	return true;
}

void
RedisManager::TaskProcessThread()
{
	std::cout << u8"Redis 스레드 시작..." << std::endl;

	while (mIsTaskRun)
	{
		bool isIdle = true;

		if (auto reqTask = PopRequestTask(); reqTask != nullptr && reqTask->TaskID != RedisTaskID::INVALID)
		{
			isIdle = false;

			if (reqTask->TaskID == RedisTaskID::REQUEST_LOGIN)
			{
				auto pRequest = reinterpret_cast<RedisLoginReq*>(reqTask->pData);

				ERROR_CODE result = ERROR_CODE::LOGIN_USER_INVALID_PW;
				
				std::string value;
				if (mConn->get(pRequest->UserID, value))
				{
					result = ERROR_CODE::NONE;

					if (value.compare(pRequest->UserPW) == 0)
					{
						result = ERROR_CODE::NONE;
					}
				}
				else
				{
					std::cerr << mConn->getErrorStr() << std::endl;
				}

				RedisLoginRes bodyData;
				bodyData.Result = static_cast<uint16_t>(result);

				RedisTaskPtr resTask = std::make_shared<RedisTask>();
				resTask->UserIndex = reqTask->UserIndex;
				resTask->TaskID = RedisTaskID::RESPONSE_LOGIN;
				resTask->DataSize = sizeof(RedisLoginRes);
				resTask->pData = new char[resTask->DataSize];
				memcpy(resTask->pData, &bodyData, resTask->DataSize);

				PushResponse(resTask);
			}

			//reqTask->Release();
		}


		if (isIdle)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}

	std::cout << u8"Redis 스레드 종료" << std::endl;
}

RedisTaskPtr
RedisManager::PopRequestTask()
{
	std::lock_guard<std::mutex> guard(mReqLock);

	if (mRequestTask.empty())
	{
		return nullptr;
	}

	auto& task = mRequestTask.front();
	mRequestTask.pop();

	return task;
}

void
RedisManager::PushResponse(const RedisTaskPtr& task)
{
	std::lock_guard<std::mutex> guard(mResLock);
	mResponseTask.push(task);
}