#include "AccountStore.h"
#include "CookHelper.h"
#include <algorithm>
#include "CookServerHelper.h"

bool AccountStore::getPublicProfile(const std::string& viewerId, const std::string& targetId, UserContactInfo& info, bool includeStatistics)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    UserAccount& viewer = ensureNoLock(viewerId);
    std::map<std::string, UserAccount>::const_iterator target = m_accounts.find(targetId);
    if (target == m_accounts.end()) return false;
    info = makeContactInfoNoLock(viewer, target->second);
    info.m_avatarUrl = target->second.m_avatarUrl;
    info.m_introduction = target->second.m_introduction;
    info.m_receivedLikeCount = 0;
    if (!includeStatistics) return true;
    syncResourceVideosNoLock(false);
    for (std::map<std::string, VideoInfo>::const_iterator video = m_videos.begin(); video != m_videos.end(); ++video)
    {
        if (video->second.m_ownerUserId != targetId || video->second.m_status != "published" || video->second.m_mediaType != "video") continue;
        const std::string key = "video:" + video->first;
        for (std::map<std::string, UserAccount>::const_iterator user = m_accounts.begin(); user != m_accounts.end(); ++user)
        {
            if (user->second.m_likedTargetIds.count(key) != 0) ++info.m_receivedLikeCount;
        }
    }
    return true;
}

std::vector<UserContactInfo> AccountStore::listFollowers(const std::string& userId)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    UserAccount& viewer = ensureNoLock(userId);
    std::vector<UserContactInfo> result;
    for (std::map<std::string, UserAccount>::const_iterator it = m_accounts.begin(); it != m_accounts.end(); ++it)
    {
        if (it->second.m_followingUserIds.count(viewer.m_userId) == 0) continue;
        UserContactInfo info = makeContactInfoNoLock(viewer, it->second);
        info.m_avatarUrl = it->second.m_avatarUrl;
        info.m_introduction = it->second.m_introduction;
        result.push_back(info);
    }
    return result;
}

std::vector<MessageInfo> AccountStore::listConversation(const std::string& userId, const std::string& peerId)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    UserAccount& viewer = ensureNoLock(userId);
    std::vector<MessageInfo> result;
    for (size_t i = 0; i < m_messages.size(); ++i)
    {
        const MessageInfo& message = m_messages[i];
        if (message.m_type == "chat" && ((message.m_fromUserId == viewer.m_userId && message.m_toUserId == peerId) ||
            (message.m_toUserId == viewer.m_userId && message.m_fromUserId == peerId))) result.push_back(message);
    }
    return result;
}

void AccountStore::readConversation(const std::string& userId, const std::string& peerId)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    UserAccount& viewer = ensureNoLock(userId);
    bool changed = false;
    for (size_t i = 0; i < m_messages.size(); ++i)
    {
        MessageInfo& message = m_messages[i];
        if (message.m_type == "chat" && message.m_toUserId == viewer.m_userId && message.m_fromUserId == peerId && !message.m_read)
        {
            message.m_read = true;
            changed = true;
        }
    }
    if (changed) saveNoLock();
}

bool AccountStore::updateIntroduction(const std::string& userId, const std::string& introduction, std::string& message)
{
    if (introduction.size() > 80 || !CookServerHelper::isUtf8Text(introduction))
    { message = "Introduction must be valid UTF-8, at most 20 characters"; return false; }
    size_t count = 0;
    for (size_t i = 0; i < introduction.size(); ++i)
    {
        const unsigned char ch = static_cast<unsigned char>(introduction[i]);
        if (ch < 32 || ch == 127) { message = "Introduction cannot contain control characters"; return false; }
        if ((ch & 0xC0) != 0x80) ++count;
    }
    if (count > 20) { message = "Introduction cannot exceed 20 characters"; return false; }
    std::lock_guard<std::mutex> lock(m_mutex);
    UserAccount& account = ensureNoLock(userId);
    if (account.m_introduction != introduction) { account.m_introduction = introduction; saveNoLock(); }
    message = "ok";
    return true;
}