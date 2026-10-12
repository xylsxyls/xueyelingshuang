#include "AccountStore.h"
#include "Config.h"
#include "RapidJson/RapidJsonAPI.h"
#include <algorithm>

bool AccountStore::listRecipeReviewHistory(const std::string &userId, const std::string &recipeId,
    const std::string &cursor, int32_t count, RapidJsonDocument &result, std::string &message)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    loadIfNeededNoLock();
    const std::map<std::string, std::vector<std::string>>::const_iterator row = m_recipeReviewOrder.find(recipeId);
    if (row == m_recipeReviewOrder.end() || row->second.empty() || count < 1)
    {
        message = "历史不存在或分页参数无效";
        return false;
    }
    const CookReviewRevision &latest = m_recipeReviews[row->second.back()];
    if (latest.m_ownerUserId != userId && !reviewerNoLock(userId))
    {
        message = "没有查看该投稿历史的权限";
        return false;
    }
    size_t end = row->second.size();
    if (!cursor.empty())
    {
        const std::vector<std::string>::const_iterator found =
            std::find(row->second.begin(), row->second.end(), cursor);
        if (found == row->second.end())
        {
            message = "历史游标无效，请刷新";
            return false;
        }
        end = static_cast<size_t>(found - row->second.begin());
    }
    const size_t maximum = static_cast<size_t>((std::min)(g_config.m_reviewPageMaxItems, count));
    const size_t first = end > maximum ? end - maximum : 0;
    RapidJsonValue history;
    history.setArray();
    for (size_t i = end; i > first; --i)
    {
        history.pushValue(m_recipeReviews[row->second[i - 1]].toJson());
    }
    result.setObject();
    result.addValue("history", history);
    result.addString("nextCursor", first == 0 ? "" : row->second[first]);
    result.addBool("hasMore", first != 0);
    message = "ok";
    return true;
}