#pragma once
#include <string>
#include <unordered_map>

#include "ErrorCode.h"

class User;

class UserManager
{
public:
	UserManager(uint32_t maxUserCount);
	~UserManager();

	int GetCurrentUserCnt() const	{ return mCurrentUserCnt; }
	int GetMaxUserCnt() const		{ return mMaxUserCnt; }
	void IncreaseUserCnt()		 	{ ++mCurrentUserCnt; }
	void DecreaseUserCnt()			{ if (mCurrentUserCnt > 0) --mCurrentUserCnt; }

	ERROR_CODE AddUser(const std::string& userID, int clientIndex);
	int FindUserIndexByID(const std::string& userID);
	void DeleteUserInfo(User* user);

	User* GetUserByConnIdx(int clientIndex) { return mUserObjPool[clientIndex]; }


private:
	uint32_t mMaxUserCnt = 0;
	int mCurrentUserCnt = 0;

	std::vector<User*> mUserObjPool; //vector·Î
	std::unordered_map<std::string, int> mUserIDDictionary;
};
