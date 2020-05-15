#include "User.h"
#include "UserManager.h"

UserManager::UserManager(uint32_t maxUserCount)
{
	mMaxUserCnt = maxUserCount;
	mUserObjPool.reserve(maxUserCount);

	for (int i = 0; i < maxUserCount; i++)
	{
		mUserObjPool.push_back(new User(i));
	}
}

UserManager::~UserManager()
{
	for (auto user : mUserObjPool)
	{
		delete user;
	}
	mUserObjPool.clear();
}

ERROR_CODE
UserManager::AddUser(const std::string& userID, int clientIndex)
{
	//TODO 최흥배 유저 중복 조사하기

	mUserObjPool[clientIndex]->Login(userID);
	mUserIDDictionary[userID] = clientIndex;

	return ERROR_CODE::NONE;
}

int
UserManager::FindUserIndexByID(const std::string& userID)
{
	auto res = mUserIDDictionary.find(userID);
	return res != mUserIDDictionary.end() ? res->second : -1;
}

void
UserManager::DeleteUserInfo(User* user)
{
	mUserIDDictionary.erase(user->GetUserId());
	user->Clear();
}