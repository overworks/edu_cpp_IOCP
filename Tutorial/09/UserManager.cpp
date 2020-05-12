#include "UserManager.h"

UserManager::~UserManager()
{
	Release();
}

void
UserManager::Init(int maxUserCount)
{
	Release();

	mUserObjPool.reserve(maxUserCount);

	for (auto i = 0; i < maxUserCount; i++)
	{
		mUserObjPool.push_back(new User(i));
	}
}

ERROR_CODE
UserManager::AddUser(const std::string& userID, int clientIndex)
{
	//TODO 최흥배 유저 중복 조사하기
	mUserObjPool[clientIndex]->Login(userID);
	mUserIDDictionary.insert(std::make_pair(userID, clientIndex));

	return ERROR_CODE::NONE;
}

int
UserManager::FindUserIndexByID(const std::string& userID) const
{
	if (auto res = mUserIDDictionary.find(userID); res != mUserIDDictionary.end())
	{
		return res->second;
	}

	return -1;
}

void
UserManager::DeleteUserInfo(User* user)
{
	mUserIDDictionary.erase(user->GetUserId());
	user->Clear();
}

void
UserManager::Release()
{
	for (size_t i = 0, imax = mUserObjPool.size(); i < imax; i++)
	{
		delete mUserObjPool[i];
	}
	mUserObjPool.clear();
}