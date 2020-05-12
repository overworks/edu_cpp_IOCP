#pragma once
#include <unordered_map>

#include "ErrorCode.h"
#include "User.h"

class UserManager
{
public:
	UserManager() = default;
	~UserManager();

	void Init(int maxUserCount);
	int GetCurrentUserCnt() const { return mCurrentUserCnt; }
	size_t GetMaxUserCnt() const { return mUserObjPool.size(); }
	void IncreaseUserCnt() { mCurrentUserCnt++; }	
	void DecreaseUserCnt() { if (mCurrentUserCnt > 0) mCurrentUserCnt--; }

	ERROR_CODE AddUser(const std::string& userID, int clientIndex);
		
	int FindUserIndexByID(const std::string& userID) const;
	void DeleteUserInfo(User* user);

	User* GetUserByConnIdx(int clientIndex) { return mUserObjPool[clientIndex]; }

private:
	void Release();

private:
	int mCurrentUserCnt = 0;

	std::vector<User*> mUserObjPool; //vector·Î
	std::unordered_map<std::string, int> mUserIDDictionary;
};
