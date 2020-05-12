#pragma once

#include "RedisTaskDefine.h"
#include "ErrorCode.h"

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <memory>

using RedisTaskPtr = std::shared_ptr<RedisTask>;

namespace RedisCpp
{
	class CRedisConn;
}

class RedisManager
{
public:
	RedisManager();
	~RedisManager();

	bool Run(const std::string& ip, uint16_t port, size_t threadCount);
	void End();

	void PushRequest(RedisTaskPtr task);
	RedisTaskPtr TakeResponseTask();

private:
	bool Connect(const std::string& ip, uint16_t port);

	void JoinThreads();
	void TaskProcessThread();

	RedisTaskPtr TakeRequestTask();
	void PushResponse(RedisTaskPtr task);


private:
	RedisCpp::CRedisConn* mConn;

	bool		mIsTaskRun = false;
	std::vector<std::thread> mTaskThreads;

	std::mutex mReqLock;
	std::queue<RedisTaskPtr> mRequestTask;

	std::mutex mResLock;
	std::queue<RedisTaskPtr> mResponseTask;
};