#include "AccountStore.h"
#include "CookHelper.h"
#include "CookRecipeConfig.h"
#include "CookRecipeDelta.h"
#include "CookRecipePresentation.h"
#include "CookResourcePaths.h"
#include "Config.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "RapidJson/RapidJsonAPI.h"
#include <algorithm>
#include <limits.h>

bool AccountStore::submitCreationNoLock(
    const std::string &userId, Recipe &recipe, const std::string &alias, bool publish, std::string &message)
{
    const std::map<std::string, UserAccount>::iterator author = m_accounts.find(userId);
    if (author == m_accounts.end() || recipe.m_authorUserId != userId ||
        !CookRecipePresentation::Validate(recipe, message) || recipe.m_tasks.empty() || recipe.m_tasks.size() > 256 ||
        !CookRecipeConfig::validateTaskGraph(recipe, &message))
    {
        message = "菜谱身份或内容无效";
        return false;
    }
    if ((recipe.m_priceType != "free" && recipe.m_priceType != "coin" && recipe.m_priceType != "yuanbao") ||
        (recipe.m_priceType == "free" ? recipe.m_priceAmount != 0 : recipe.m_priceAmount <= 0) ||
        recipe.m_mediaIds.size() > 9)
    {
        message = "菜谱价格或媒体数量无效";
        return false;
    }
    recipe.m_priceCoins = recipe.m_priceType == "coin" ? recipe.m_priceAmount : 0;
    recipe.m_defaultOwned = recipe.m_priceType == "free";
    std::set<std::string> media;
    std::map<std::string, VideoInfo> mediaBefore;
    for (size_t i = 0; i < recipe.m_mediaIds.size(); ++i)
    {
        const std::map<std::string, VideoInfo>::const_iterator file = m_videos.find(recipe.m_mediaIds[i]);
        if (!media.insert(recipe.m_mediaIds[i]).second || file == m_videos.end() ||
            file->second.m_ownerUserId != userId ||
            (file->second.m_mediaType != "video" && file->second.m_mediaType != "image") ||
            (recipe.m_mediaIds.size() > 1 && file->second.m_mediaType != "image") ||
            !CSystem::DirOrFileExist(CookResourcePaths::mediaPath(file->second.m_filePath)))
        {
            message = "原始媒体必须已上传并属于当前作者，图片组最多九张";
            return false;
        }
        mediaBefore[file->first] = file->second;
    }
    if (!recipe.m_coverImageId.empty())
    {
        const std::map<std::string, VideoInfo>::const_iterator cover = m_videos.find(recipe.m_coverImageId);
        if (cover == m_videos.end() || cover->second.m_mediaType != "image" || cover->second.m_ownerUserId != userId ||
            !CookRecipePresentation::IsSquareImage(CookResourcePaths::mediaPath(cover->second.m_filePath)))
        {
            message = "菜谱小封面必须是作者上传的正方形图片";
            return false;
        }
        mediaBefore[cover->first] = cover->second;
    }
    const Recipe *oldPublicPointer = findBaseRecipeNoLock(recipe.m_id);
    const bool hadPublic = m_customRecipes.count(recipe.m_id) != 0;
    const Recipe oldPublic = oldPublicPointer == nullptr ? Recipe() : *oldPublicPointer;
    const bool hadDraft = m_creationDrafts.count(recipe.m_id) != 0;
    const Recipe oldDraft = hadDraft ? m_creationDrafts[recipe.m_id] : Recipe();
    const UserAccount oldAccount = author->second;
    const std::string aliasKey = userId + "|" + alias;
    const bool hadAlias = !alias.empty() && m_creationAliases.count(aliasKey) != 0;
    const std::string oldAlias = hadAlias ? m_creationAliases[aliasKey] : "";
    if (!alias.empty() && hadAlias && oldAlias != recipe.m_id)
    {
        message = "客户端创建标识已用于另一道菜";
        return false;
    }
    if (!publish)
    {
        const std::string pendingId = hadDraft ? oldDraft.m_currentRevisionId : "";
        const bool supersede = m_recipeReviews.count(pendingId) != 0 && m_recipeReviews[pendingId].m_state == "pending";
        if (supersede)
        {
            m_recipeReviews[pendingId].m_state = "superseded";
        }
        recipe.m_status = "draft";
        recipe.m_currentRevisionId = "draft_" + CSystem::uuid(0);
        m_creationDrafts[recipe.m_id] = recipe;
        author->second.m_customRecipeIds.insert(recipe.m_id);
        author->second.m_ownedRecipeIds.insert(recipe.m_id);
        if (!alias.empty())
        {
            m_creationAliases[aliasKey] = recipe.m_id;
        }
        if (!saveNoLock(true))
        {
            if (supersede)
            {
                m_recipeReviews[pendingId].m_state = "pending";
            }
            if (hadDraft)
            {
                m_creationDrafts[recipe.m_id] = oldDraft;
            }
            else
            {
                m_creationDrafts.erase(recipe.m_id);
            }
            author->second = oldAccount;
            if (!alias.empty() && !hadAlias)
            {
                m_creationAliases.erase(aliasKey);
            }
            saveNoLock(true);
            message = "草稿未能保存，修改仍保留在客户端";
            return false;
        }
        decorateCreationNoLock(userId, recipe);
        message = "草稿已保存";
        return true;
    }
    const size_t oldOrderSize =
        m_recipeReviewOrder.count(recipe.m_id) == 0 ? 0 : m_recipeReviewOrder[recipe.m_id].size();
    if (oldOrderSize >= INT32_MAX)
    {
        message = "菜谱版本数量已达上限";
        return false;
    }
    Recipe previous;
    CookReviewRevision oldReview;
    const std::string parent = oldOrderSize == 0 ? "" : m_recipeReviewOrder[recipe.m_id].back();
    if (!parent.empty())
    {
        oldReview = m_recipeReviews[parent];
        if (!restoreReviewNoLock(parent, previous, message))
        {
            return false;
        }
    }
    else
    {
        previous = oldPublicPointer == nullptr ? recipe : oldPublic;
        decorateRecipeMediaNoLock(previous);
        if (previous.m_mediaIds.empty() && !previous.m_videoId.empty())
        {
            previous.m_mediaIds.push_back(previous.m_videoId);
        }
    }
    RapidJsonDocument delta;
    if (!CookRecipeDelta::compare(recipeToStorageJsonNoLock(previous), recipeToStorageJsonNoLock(recipe), delta))
    {
        message = "无法生成菜谱差异记录";
        return false;
    }
    if (!parent.empty() && CookRecipeDelta::empty(delta))
    {
        recipe = previous;
        decorateCreationNoLock(userId, recipe);
        message = "内容没有变化，未重复提交或扣除次数";
        return true;
    }
    const std::string day = submissionDayKeyNoLock(userId);
    const bool hadUsage = m_submissionUsage.count(day) != 0;
    const std::map<std::string, int32_t> oldUsage =
        hadUsage ? m_submissionUsage[day] : std::map<std::string, int32_t>();
    std::map<std::string, int32_t> usage = oldUsage;
    const bool isChange = oldOrderSize != 0 || (oldPublicPointer != nullptr && oldPublic.m_status == "published");
    const int32_t usedChanges = usage.count(recipe.m_id) == 0 ? 0 : usage[recipe.m_id];
    if (!reviewerNoLock(userId))
    {
        if (usage.count(recipe.m_id) == 0 && usage.size() >= static_cast<size_t>(g_config.m_recipeDailyDistinctLimit))
        {
            message = "今天已提交或修改" + CStringManager::toStringInt32(g_config.m_recipeDailyDistinctLimit) +
                      "道菜，请明天再提交";
            return false;
        }
        if (isChange && usedChanges >= g_config.m_recipeDailyChangesLimit)
        {
            message = "这道菜今天已修改" + CStringManager::toStringInt32(g_config.m_recipeDailyChangesLimit) +
                      "次，请明天再修改";
            return false;
        }
        usage[recipe.m_id] = usedChanges + (isChange ? 1 : 0);
    }
    std::map<std::string, PersonalizationConfig> personal, shared, personalBefore, sharedBefore;
    if (!preparePublicationNoLock(recipe, personal, shared, message))
    {
        return false;
    }
    for (std::map<std::string, PersonalizationConfig>::const_iterator it = personal.begin(); it != personal.end(); ++it)
    {
        personalBefore[it->first] = m_personalizations[it->first];
    }
    for (std::map<std::string, PersonalizationConfig>::const_iterator it = shared.begin(); it != shared.end(); ++it)
    {
        sharedBefore[it->first] = m_sharedPersonalizations[it->first];
    }
    CookReviewRevision review;
    review.m_id = "review_" + CSystem::uuid(0);
    review.m_recipeId = recipe.m_id;
    review.m_ownerUserId = userId;
    review.m_parentId = parent;
    review.m_previousApprovedId =
        oldPublicPointer != nullptr && oldPublic.m_status == "published"
            ? (m_recipeReviews.count(oldPublic.m_currentRevisionId) != 0 ? oldPublic.m_currentRevisionId : "baseline")
            : "";
    review.m_deltaJson = delta.toString();
    review.m_requiresReview = recipe.m_priceType != "free";
    review.m_state = review.m_requiresReview ? "pending" : "approved";
    review.m_submissionNumber = static_cast<int32_t>(oldOrderSize + 1);
    review.m_submittedAtSeconds = nowSecondsNoLock();
    if (review.m_requiresReview)
    {
        review.m_reviewNumber = 1;
        if (oldOrderSize != 0)
        {
            const std::vector<std::string> &order = m_recipeReviewOrder[recipe.m_id];
            for (size_t i = 0; i < order.size(); ++i)
            {
                review.m_reviewNumber += m_recipeReviews[order[i]].m_requiresReview ? 1 : 0;
            }
        }
    }
    if (review.m_id.size() != 39)
    {
        message = "无法生成安全的提交版本标识";
        return false;
    }
    recipe.m_status = review.m_requiresReview ? "reviewing" : "published";
    recipe.m_currentRevisionId = review.m_id;
    const bool hadBase = m_reviewBases.count(recipe.m_id) != 0;
    if (!hadBase)
    {
        m_reviewBases[recipe.m_id] = recipeToStorageJsonNoLock(previous);
    }
    if (!parent.empty() && oldReview.m_state == "pending")
    {
        m_recipeReviews[parent].m_state = "superseded";
    }
    m_recipeReviews[review.m_id] = review;
    m_recipeReviewOrder[recipe.m_id].push_back(review.m_id);
    m_creationDrafts[recipe.m_id] = recipe;
    if (!review.m_requiresReview)
    {
        m_customRecipes[recipe.m_id] = recipe;
        writePersonalizationsNoLock(personal, shared);
    }
    else if (oldPublicPointer != nullptr && oldPublic.m_status == "published" && oldPublic.m_mediaIds.empty())
    {
        // 固定旧版视频引用，不能因为同作者随后上传新视频而改变已通过版封面/播放器。
        Recipe pinned = oldPublic;
        decorateRecipeMediaNoLock(pinned);
        if (!pinned.m_videoId.empty())
        {
            pinned.m_mediaIds.push_back(pinned.m_videoId);
        }
        m_customRecipes[recipe.m_id] = pinned;
    }
    author->second.m_customRecipeIds.insert(recipe.m_id);
    author->second.m_ownedRecipeIds.insert(recipe.m_id);
    if (!alias.empty())
    {
        m_creationAliases[aliasKey] = recipe.m_id;
    }
    if (!reviewerNoLock(userId))
    {
        m_submissionUsage[day] = usage;
    }
    for (std::map<std::string, VideoInfo>::const_iterator it = mediaBefore.begin(); it != mediaBefore.end(); ++it)
    {
        VideoInfo &file = m_videos[it->first];
        if (std::find(file.m_recipeIds.begin(), file.m_recipeIds.end(), recipe.m_id) == file.m_recipeIds.end())
        {
            file.m_recipeIds.push_back(recipe.m_id);
        }
        if (!review.m_requiresReview)
        {
            file.m_status = "published";
        }
        else if (file.m_status == "uploaded")
        {
            file.m_status = "reviewing";
        }
    }
    if (!saveNoLock(true))
    {
        if (hadDraft)
        {
            m_creationDrafts[recipe.m_id] = oldDraft;
        }
        else
        {
            m_creationDrafts.erase(recipe.m_id);
        }
        if (hadPublic)
        {
            m_customRecipes[recipe.m_id] = oldPublic;
        }
        else
        {
            m_customRecipes.erase(recipe.m_id);
        }
        if (!hadBase)
        {
            m_reviewBases.erase(recipe.m_id);
        }
        m_recipeReviews.erase(review.m_id);
        if (!parent.empty())
        {
            m_recipeReviews[parent] = oldReview;
        }
        m_recipeReviewOrder[recipe.m_id].resize(oldOrderSize);
        if (oldOrderSize == 0)
        {
            m_recipeReviewOrder.erase(recipe.m_id);
        }
        if (hadUsage)
        {
            m_submissionUsage[day] = oldUsage;
        }
        else
        {
            m_submissionUsage.erase(day);
        }
        author->second = oldAccount;
        if (!alias.empty() && !hadAlias)
        {
            m_creationAliases.erase(aliasKey);
        }
        writePersonalizationsNoLock(personalBefore, sharedBefore);
        for (std::map<std::string, VideoInfo>::const_iterator it = mediaBefore.begin(); it != mediaBefore.end(); ++it)
        {
            m_videos[it->first] = it->second;
        }
        saveNoLock(true);
        message = "提交未能保存，未消耗本次额度，请重试";
        return false;
    }
    decorateCreationNoLock(userId, recipe);
    decorateRecipeMediaNoLock(recipe);
    message = review.m_requiresReview ? "已提交，审核中" : "免费菜谱已发布";
    return true;
}