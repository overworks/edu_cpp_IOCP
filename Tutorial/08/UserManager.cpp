#include <utility>
#include "User.h"
#include "UserManager.h"

UserManager::UserManager(int maxUserCount)
	: mMaxUserCnt(maxUserCount)
{
	mUserObjPool.reserve(mMaxUserCnt);

	for (auto i = 0; i < mMaxUserCnt; i++)
	{
		auto userPtr = std::make_shared<User>(i);
		mUserObjPool.push_back(userPtr);
	}
}

ERROR_CODE
UserManager::AddUser(const std::string& userID, int clientIndex)
{
	//TODO 최흥배 유저 중복 조사하기

	mUserObjPool[clientIndex]->SetLogin(userID);
	mUserIDDictionary.insert(std::make_pair(userID, clientIndex));

	return ERROR_CODE::NONE;
}

int
UserManager::FindUserIndexByID(const std::string& userID)
{
	if (auto res = mUserIDDictionary.find(userID); res != mUserIDDictionary.end())
	{
		return (*res).second;
	}

	return -1;
}

void
UserManager::DeleteUserInfo(User* user)
{
	mUserIDDictionary.erase(user->GetUserId());
	user->Clear();
}

