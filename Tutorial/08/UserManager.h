#pragma once

#include <memory>
#include <unordered_map>

#include "ErrorCode.h"

class User;

using UserPtr = std::shared_ptr<User>;

class UserManager
{
public:
	UserManager(int maxUserCount);
	~UserManager() = default;

	int GetCurrentUserCnt() const { return mCurrentUserCnt; }
	int GetMaxUserCnt() const { return mMaxUserCnt; }
	void IncreaseUserCnt() { ++mCurrentUserCnt; }
	void DecreaseUserCnt() { if (mCurrentUserCnt > 0) --mCurrentUserCnt; }
	ERROR_CODE AddUser(const std::string& userID, int clientIndex);
	int FindUserIndexByID(const std::string& userID);
	void DeleteUserInfo(User* user);

	UserPtr GetUserByConnIdx(UINT32 clientIndex) { return mUserObjPool[clientIndex]; }
	const UserPtr GetUserByConnIdx(UINT32 clientIndex) const { return mUserObjPool[clientIndex]; }

private:
	int mMaxUserCnt = 0;
	int mCurrentUserCnt = 0;

	std::vector<UserPtr> mUserObjPool;
	std::unordered_map<std::string, int> mUserIDDictionary;
};
