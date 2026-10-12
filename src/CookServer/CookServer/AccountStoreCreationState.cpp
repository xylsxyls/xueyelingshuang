#include "AccountStore.h"
#include "CookCreationFlow.h"
#include "CookHelper.h"
#include "CookRecipeConfig.h"
#include "CookRecipeDelta.h"
#include "Config.h"
#include "CStringManager/CStringManagerAPI.h"
#include "RapidJson/RapidJsonAPI.h"
#include <algorithm>

bool AccountStore::reviewerNoLock(const std::string &userId) const
{
    const std::map<std::string, UserAccount>::const_iterator found = m_accounts.find(userId);
    return found != m_accounts.end() && (found->second.m_role == "admin" || found->second.m_role == "super_admin");
}

const Recipe *AccountStore::findCreationRecipeNoLock(const std::string &userId, const std::string &recipeId) const
{
    const std::map<std::string, Recipe>::const_iterator draft = m_creationDrafts.find(recipeId);
    if (draft != m_creationDrafts.end() && draft->second.m_authorUserId == userId)
    {
        return &draft->second;
    }
    const Recipe *original = findBaseRecipeNoLock(recipeId);
    return original != nullptr && original->m_authorUserId == userId ? original : nullptr;
}

std::string AccountStore::submissionDayKeyNoLock(const std::string &userId) const
{
    const int64_t day =
        (static_cast<int64_t>(nowSecondsNoLock()) + static_cast<int64_t>(g_config.m_submissionTimezoneMinutes) * 60) /
        86400;
    return userId + "|" + CStringManager::toStringInt64(day);
}

void AccountStore::decorateCreationNoLock(const std::string &userId, Recipe &recipe)
{
    if (recipe.m_authorUserId != userId)
    {
        return;
    }
    const std::map<std::string, CookReviewRevision>::const_iterator review =
        m_recipeReviews.find(recipe.m_currentRevisionId);
    if (review != m_recipeReviews.end())
    {
        recipe.m_reviewState = review->second.m_requiresReview ? review->second.m_state : "published";
        recipe.m_reviewId = review->second.m_id;
        recipe.m_reviewNumber = review->second.m_reviewNumber;
        recipe.m_reviewReason = review->second.m_reason;
        recipe.m_submittedAtSeconds = review->second.m_submittedAtSeconds;
        recipe.m_reviewedAtSeconds = review->second.m_reviewedAtSeconds;
    }
    else
    {
        recipe.m_reviewState = recipe.m_status == "draft" ? "draft" : "published";
    }
    recipe.m_submissionLimited = !reviewerNoLock(userId);
    if (!recipe.m_submissionLimited)
    {
        return;
    }
    const std::map<std::string, std::map<std::string, int32_t>>::const_iterator row =
        m_submissionUsage.find(submissionDayKeyNoLock(userId));
    const size_t distinct = row == m_submissionUsage.end() ? 0 : row->second.size();
    int32_t changes = 0;
    if (row != m_submissionUsage.end())
    {
        const std::map<std::string, int32_t>::const_iterator item = row->second.find(recipe.m_id);
        if (item != row->second.end())
        {
            changes = item->second;
        }
    }
    recipe.m_changesRemaining = (std::max)(0, g_config.m_recipeDailyChangesLimit - changes);
    recipe.m_dailyRecipesRemaining =
        (std::max)(0, g_config.m_recipeDailyDistinctLimit - static_cast<int32_t>(distinct));
}

bool AccountStore::preparePublicationNoLock(const Recipe &recipe,
    std::map<std::string, PersonalizationConfig> &personal, std::map<std::string, PersonalizationConfig> &shared,
    std::string &message) const
{
    const Recipe *original = findBaseRecipeNoLock(recipe.m_id);
    if (original == nullptr)
    {
        return true;
    }
    // 只准备更新，不在审核中更改用户仍在使用的公开版个性化。
    const auto prepare = [&](const std::map<std::string, PersonalizationConfig> &stored,
                             std::map<std::string, PersonalizationConfig> &output) -> bool
    {
        for (std::map<std::string, PersonalizationConfig>::const_iterator it = stored.begin(); it != stored.end(); ++it)
        {
            if (it->second.m_recipeId != recipe.m_id)
            {
                continue;
            }
            PersonalizationConfig updated;
            if (!CookCreationFlow::rebasePersonalization(*original, recipe, it->second, updated, message))
            {
                return false;
            }
            output[it->first] = updated;
        }
        return true;
    };
    return prepare(m_personalizations, personal) && prepare(m_sharedPersonalizations, shared);
}

void AccountStore::writePersonalizationsNoLock(const std::map<std::string, PersonalizationConfig> &personal,
    const std::map<std::string, PersonalizationConfig> &shared)
{
    for (std::map<std::string, PersonalizationConfig>::const_iterator it = personal.begin(); it != personal.end(); ++it)
    {
        m_personalizations[it->first] = it->second;
    }
    for (std::map<std::string, PersonalizationConfig>::const_iterator it = shared.begin(); it != shared.end(); ++it)
    {
        m_sharedPersonalizations[it->first] = it->second;
    }
}

bool AccountStore::readableMediaNoLock(const std::string &userId, const VideoInfo &video) const
{
    if (video.m_status == "published")
    {
        return true;
    }
    if (userId.empty())
    {
        return false;
    }
    if (video.m_ownerUserId == userId)
    {
        return true;
    }
    if (reviewerNoLock(userId))
    {
        for (size_t i = 0; i < video.m_recipeIds.size(); ++i)
        {
            const std::map<std::string, std::vector<std::string>>::const_iterator reviews =
                m_recipeReviewOrder.find(video.m_recipeIds[i]);
            if (reviews != m_recipeReviewOrder.end() && !reviews->second.empty())
            {
                return true;
            }
        }
    }
    return false;
}

bool AccountStore::restoreReviewNoLock(const std::string &reviewId, Recipe &recipe, std::string &message) const
{
    const std::map<std::string, CookReviewRevision>::const_iterator record = m_recipeReviews.find(reviewId);
    if (record == m_recipeReviews.end())
    {
        message = "Review version does not exist";
        return false;
    }
    const std::string &id = record->second.m_recipeId;
    const std::map<std::string, Recipe>::const_iterator latest = m_creationDrafts.find(id),
                                                        published = m_customRecipes.find(id);
    if (latest != m_creationDrafts.end() && latest->second.m_currentRevisionId == reviewId)
    {
        recipe = latest->second;
        return true;
    }
    if (published != m_customRecipes.end() && published->second.m_currentRevisionId == reviewId)
    {
        recipe = published->second;
        return true;
    }
    const std::map<std::string, std::string>::const_iterator base = m_reviewBases.find(id);
    const std::map<std::string, std::vector<std::string>>::const_iterator order = m_recipeReviewOrder.find(id);
    if (base == m_reviewBases.end() || order == m_recipeReviewOrder.end())
    {
        message = "Review baseline or history is missing";
        return false;
    }
    std::string json = base->second;
    for (size_t i = 0; i < order->second.size(); ++i)
    {
        const std::map<std::string, CookReviewRevision>::const_iterator item = m_recipeReviews.find(order->second[i]);
        std::string next;
        if (item == m_recipeReviews.end() || !CookRecipeDelta::apply(json, item->second.m_deltaJson, next))
        {
            message = "Cannot restore review history";
            return false;
        }
        json.swap(next);
        if (item->first == reviewId)
        {
            if (!CookRecipeConfig::loadRecipeFromJson(json, recipe, &message))
            {
                return false;
            }
            recipe.m_currentRevisionId = reviewId;
            recipe.m_status = item->second.m_state == "approved"   ? "published"
                              : item->second.m_state == "rejected" ? "rejected"
                                                                   : "reviewing";
            return true;
        }
    }
    message = "Review is not in its recipe history";
    return false;
}

bool AccountStore::rebuildReviewIndexesNoLock()
{
    m_recipeReviewOrder.clear();
    for (std::map<std::string, CookReviewRevision>::const_iterator it = m_recipeReviews.begin();
        it != m_recipeReviews.end(); ++it)
    {
        m_recipeReviewOrder[it->second.m_recipeId].push_back(it->first);
    }
    for (std::map<std::string, std::vector<std::string>>::iterator row = m_recipeReviewOrder.begin();
        row != m_recipeReviewOrder.end(); ++row)
    {
        // 比较服务端提交序号，不依赖随机UUID字典序。
        std::sort(row->second.begin(), row->second.end(), [&](const std::string &a, const std::string &b) -> bool
            { return m_recipeReviews[a].m_submissionNumber < m_recipeReviews[b].m_submissionNumber; });
        if (m_reviewBases.count(row->first) == 0)
        {
            return false;
        }
        std::string json = m_reviewBases[row->first];
        int32_t reviewNumber = 0;
        for (size_t i = 0; i < row->second.size(); ++i)
        {
            const CookReviewRevision &item = m_recipeReviews[row->second[i]];
            std::string next;
            if (item.m_submissionNumber != static_cast<int32_t>(i + 1) ||
                item.m_parentId != (i == 0 ? "" : row->second[i - 1]) ||
                (item.m_requiresReview ? item.m_reviewNumber != ++reviewNumber : item.m_reviewNumber != 0) ||
                !CookRecipeDelta::apply(json, item.m_deltaJson, next))
            {
                return false;
            }
            json.swap(next);
            Recipe restored;
            std::string message;
            if (!CookRecipeConfig::loadRecipeFromJson(json, restored, &message) || restored.m_id != item.m_recipeId ||
                restored.m_authorUserId != item.m_ownerUserId ||
                !CookRecipeConfig::validateTaskGraph(restored, &message))
            {
                return false;
            }
        }
    }
    return true;
}