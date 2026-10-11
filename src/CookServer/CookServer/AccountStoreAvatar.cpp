#include "AccountStore.h"
#include "CookAvatarHelper.h"

bool AccountStore::updateAvatar(const std::string& userId, const std::string& imageKey,
    std::string& oldKey, std::string& message)
{
    if (!CookAvatarHelper::isKey(imageKey))
    {
        message = "Invalid avatar key";
        return false;
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    loadIfNeededNoLock();
    std::map<std::string, UserAccount>::iterator account = m_accounts.find(userId);
    if (account == m_accounts.end())
    {
        message = "User not found";
        return false;
    }
    const std::string prefix = "/api/users/avatar?userId=" + userId + "&key=";
    const std::string previous = account->second.m_avatarUrl;
    oldKey = previous.compare(0, prefix.size(), prefix) == 0 ? previous.substr(prefix.size()) : "";
    account->second.m_avatarUrl = prefix + imageKey;
    if (!saveNoLock(true))
    {
        account->second.m_avatarUrl = previous;
        saveNoLock(true);
        oldKey.clear();
        message = "Cannot persist avatar reference";
        return false;
    }
    message = "ok";
    return true;
}

bool AccountStore::getAvatarKey(const std::string& userId, std::string& imageKey)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    loadIfNeededNoLock();
    const std::map<std::string, UserAccount>::const_iterator account = m_accounts.find(userId);
    if (account == m_accounts.end())
    {
        return false;
    }
    const std::string prefix = "/api/users/avatar?userId=" + userId + "&key=";
    const std::string& url = account->second.m_avatarUrl;
    if (url.compare(0, prefix.size(), prefix) != 0)
    {
        return false;
    }
    imageKey = url.substr(prefix.size());
    return CookAvatarHelper::isKey(imageKey);
}