#pragma once

#include "Packet.h"
#include "RedisTaskDefine.h"
#include "ErrorCode.h"


#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <memory>

namespace RedisCpp
{
	class CRedisConn;
}

using RedisTaskPtr = std::shared_ptr<RedisTask>;

class RedisManager
{
public:
	RedisManager();
	RedisManager(const RedisManager&) = delete;
	~RedisManager();

	bool Run(const std::string& host, uint16_t port, size_t threadCount);
	void End();
	void PushRequest(const RedisTaskPtr& task);

	RedisTaskPtr PopResponseTask();

private:
	bool Connect(const std::string& host, uint16_t port);

	void TaskProcessThread();

	RedisTaskPtr PopRequestTask();

	void PushResponse(const RedisTaskPtr& task);

private:
	RedisCpp::CRedisConn* mConn;

	bool mIsTaskRun = false;
	std::vector<std::thread> mTaskThreads;

	std::mutex mReqLock;
	std::queue<RedisTaskPtr> mRequestTask;

	std::mutex mResLock;
	std::queue<RedisTaskPtr> mResponseTask;
};