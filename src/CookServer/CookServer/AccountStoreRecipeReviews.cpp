#include "AccountStore.h"
#include "CookRecipeConfig.h"
#include "CookRecipeDelta.h"
#include "CookServerHelper.h"
#include "Config.h"
#include "RapidJson/RapidJsonAPI.h"
#include <algorithm>
#include <limits.h>

bool AccountStore::listRecipeReviews(const std::string &userId, const std::string &state, const std::string &cursor,
    int32_t count, RapidJsonDocument &result, std::string &message)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    loadIfNeededNoLock();
    if (!reviewerNoLock(userId) || (state != "pending" && state != "processed"))
    {
        message = "仅管理员可以查看审核队列";
        return false;
    }
    std::vector<const CookReviewRevision *> rows;
    for (std::map<std::string, CookReviewRevision>::const_iterator it = m_recipeReviews.begin();
        it != m_recipeReviews.end(); ++it)
    {
        const CookReviewRevision &item = it->second;
        if (item.m_requiresReview &&
            (state == "pending" ? item.m_state == "pending" : item.m_state == "approved" || item.m_state == "rejected"))
        {
            rows.push_back(&item);
        }
    }
    // 已处理以实际决定时刻排序，同秒再按持久序列和版本ID稳定排序。
    std::sort(rows.begin(), rows.end(),
        [&](const CookReviewRevision *a, const CookReviewRevision *b) -> bool
        {
            const int32_t first = state == "pending" ? a->m_submittedAtSeconds : a->m_reviewedAtSeconds;
            const int32_t second = state == "pending" ? b->m_submittedAtSeconds : b->m_reviewedAtSeconds;
            if (first != second)
            {
                return first > second;
            }
            if (state == "processed" && a->m_decisionSequence != b->m_decisionSequence)
            {
                return a->m_decisionSequence > b->m_decisionSequence;
            }
            return a->m_id > b->m_id;
        });
    size_t begin = 0;
    if (!cursor.empty())
    {
        while (begin < rows.size() && rows[begin]->m_id != cursor)
        {
            ++begin;
        }
        if (begin == rows.size())
        {
            message = "审核列表已变化，请刷新首页";
            return false;
        }
        ++begin;
    }
    const size_t maximum = static_cast<size_t>((std::max)(1, (std::min)(g_config.m_reviewPageMaxItems, count)));
    const size_t end = (std::min)(rows.size(), begin + maximum);
    RapidJsonValue items;
    items.setArray();
    for (size_t i = begin; i < end; ++i)
    {
        Recipe recipe;
        if (!restoreReviewNoLock(rows[i]->m_id, recipe, message))
        {
            return false;
        }
        decorateRecipeMediaNoLock(recipe);
        RapidJsonValue item = rows[i]->toJson();
        CookServerHelper::addString(item, "title", recipe.m_title);
        CookServerHelper::addString(item, "subtitle", recipe.m_subtitle);
        CookServerHelper::addString(item, "author", recipe.m_author);
        item.addString("videoId", recipe.m_videoId);
        item.addString("coverImageId", recipe.m_coverImageId);
        item.addStringArray("mediaIds", recipe.m_mediaIds);
        item.addString("priceType", recipe.m_priceType);
        item.addInt("priceAmount", recipe.m_priceAmount);
        items.pushValue(item);
    }
    result.setObject();
    result.addValue("items", items);
    result.addString("state", state);
    result.addString("nextCursor", end < rows.size() && end > begin ? rows[end - 1]->m_id : "");
    result.addBool("hasMore", end < rows.size());
    message = "ok";
    return true;
}

bool AccountStore::getRecipeReview(const std::string &userId, const std::string &recipeId, const std::string &reviewId,
    RapidJsonDocument &result, std::string &message)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    loadIfNeededNoLock();
    std::string selected = reviewId;
    if (selected.empty())
    {
        const std::map<std::string, std::vector<std::string>>::const_iterator row = m_recipeReviewOrder.find(recipeId);
        if (row == m_recipeReviewOrder.end() || row->second.empty())
        {
            message = "这道菜尚无投稿历史";
            return false;
        }
        selected = row->second.back();
    }
    const std::map<std::string, CookReviewRevision>::const_iterator record = m_recipeReviews.find(selected);
    if (record == m_recipeReviews.end() || (!recipeId.empty() && record->second.m_recipeId != recipeId) ||
        (record->second.m_ownerUserId != userId && !reviewerNoLock(userId)))
    {
        message = "没有查看这条审核的权限";
        return false;
    }
    Recipe recipe, previous;
    if (!restoreReviewNoLock(selected, recipe, message))
    {
        return false;
    }
    result.setObject();
    RapidJsonValue recordJson = record->second.toJson();
    const std::map<std::string, UserAccount>::const_iterator reviewer =
        m_accounts.find(record->second.m_reviewerUserId);
    CookServerHelper::addString(
        recordJson, "reviewerName", reviewer == m_accounts.end() ? "" : reviewer->second.m_nickname);
    result.addValue("review", recordJson);
    decorateRecipeMediaNoLock(recipe);
    decorateCreationNoLock(recipe.m_authorUserId, recipe);
    result.addValue("recipe", CookServerHelper::recipeToJson(recipe, &m_accounts[userId], true));
    result.addBool(
        "canDecide", reviewerNoLock(userId) && record->second.m_requiresReview && record->second.m_state == "pending");
    result.addBool("hasPreviousApproved", !record->second.m_previousApprovedId.empty());
    if (!record->second.m_previousApprovedId.empty())
    {
        if (record->second.m_previousApprovedId == "baseline")
        {
            const std::map<std::string, std::string>::const_iterator base =
                m_reviewBases.find(record->second.m_recipeId);
            if (base == m_reviewBases.end() || !CookRecipeConfig::loadRecipeFromJson(base->second, previous, &message))
            {
                return false;
            }
        }
        else if (!restoreReviewNoLock(record->second.m_previousApprovedId, previous, message))
        {
            return false;
        }
        decorateRecipeMediaNoLock(previous);
        result.addValue("previousApprovedRecipe", CookServerHelper::recipeToJson(previous, &m_accounts[userId], true));
        RapidJsonDocument changes;
        if (!CookRecipeDelta::compare(recipeToStorageJsonNoLock(previous), recipeToStorageJsonNoLock(recipe), changes))
        {
            message = "无法比较上次通过版本";
            return false;
        }
        RapidJsonDocument holder;
        holder.parse("{\"value\":" + changes.toString() + "}");
        result.addValue("changes", holder.getValue("value"));
    }
    const std::vector<std::string> &history = m_recipeReviewOrder[record->second.m_recipeId];
    RapidJsonValue values;
    values.setArray();
    // 历史页首批有界，后续由history接口按游标继续取，不一次传全部旧内容。
    const size_t first = history.size() > static_cast<size_t>(g_config.m_reviewPageMaxItems)
                             ? history.size() - g_config.m_reviewPageMaxItems
                             : 0;
    for (size_t i = history.size(); i > first; --i)
    {
        values.pushValue(m_recipeReviews[history[i - 1]].toJson());
    }
    result.addValue("history", values);
    result.addBool("historyHasMore", first != 0);
    result.addString("historyCursor", first == 0 ? "" : history[first]);
    message = "ok";
    return true;
}

bool AccountStore::decideRecipeReview(const std::string &userId, const std::string &reviewId,
    const std::string &decision, const std::string &reason, RapidJsonDocument &result, std::string &message)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    loadIfNeededNoLock();
    result.setObject();
    if (!reviewerNoLock(userId))
    {
        result.addString("code", "forbidden");
        message = "只有管理员或超级管理员可以审核";
        return false;
    }
    if ((decision != "approved" && decision != "rejected") ||
        reason.size() > static_cast<size_t>(g_config.m_reviewReasonMaxBytes) || !CookServerHelper::isUtf8Text(reason) ||
        reason.find('\0') != std::string::npos)
    {
        result.addString("code", "invalid");
        message = "审核结果或说明无效";
        return false;
    }
    for (size_t i = 0; i < reason.size(); ++i)
    {
        const unsigned char c = static_cast<unsigned char>(reason[i]);
        if (c < 32 && c != '\t' && c != '\n' && c != '\r')
        {
            result.addString("code", "invalid");
            message = "审核说明含无效控制字符";
            return false;
        }
    }
    const std::map<std::string, CookReviewRevision>::iterator current = m_recipeReviews.find(reviewId);
    if (current == m_recipeReviews.end() || !current->second.m_requiresReview)
    {
        result.addString("code", "not_found");
        message = "待审版本不存在";
        return false;
    }
    if (current->second.m_state != "pending" || m_recipeReviewOrder[current->second.m_recipeId].back() != reviewId)
    {
        result.addValue("review", current->second.toJson());
        result.addString("code", current->second.m_state == "superseded" ? "version_replaced" : "already_processed");
        message = current->second.m_state == "superseded" ? "作者已提交新版本，本次操作未生效"
                                                          : "该版本已被审核，本次操作未生效";
        return false;
    }
    Recipe recipe;
    if (!restoreReviewNoLock(reviewId, recipe, message))
    {
        result.addString("code", "invalid_history");
        return false;
    }
    std::map<std::string, PersonalizationConfig> personal, shared, personalBefore, sharedBefore;
    if (decision == "approved" && !preparePublicationNoLock(recipe, personal, shared, message))
    {
        result.addString("code", "personalization_conflict");
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
    const CookReviewRevision before = current->second;
    const std::string id = current->second.m_recipeId;
    const bool hadPublic = m_customRecipes.count(id) != 0, hadDraft = m_creationDrafts.count(id) != 0;
    const Recipe oldPublic = hadPublic ? m_customRecipes[id] : Recipe(),
                 oldDraft = hadDraft ? m_creationDrafts[id] : Recipe();
    const int32_t oldSequence = m_sequence;
    if (m_sequence == INT32_MAX)
    {
        message = "审核序列已达上限";
        return false;
    }
    current->second.m_state = decision;
    current->second.m_reason = decision == "rejected" ? reason : "";
    current->second.m_reviewerUserId = userId;
    current->second.m_reviewedAtSeconds = nowSecondsNoLock();
    current->second.m_decisionSequence = ++m_sequence;
    recipe.m_status = decision == "approved" ? "published" : "rejected";
    if (hadDraft && m_creationDrafts[id].m_currentRevisionId == reviewId)
    {
        m_creationDrafts[id] = recipe;
    }
    std::map<std::string, VideoInfo> mediaBefore;
    if (decision == "approved")
    {
        m_customRecipes[id] = recipe;
        writePersonalizationsNoLock(personal, shared);
        std::vector<std::string> media = recipe.m_mediaIds;
        if (!recipe.m_coverImageId.empty())
        {
            media.push_back(recipe.m_coverImageId);
        }
        for (size_t i = 0; i < media.size(); ++i)
        {
            if (m_videos.count(media[i]) != 0)
            {
                mediaBefore[media[i]] = m_videos[media[i]];
                m_videos[media[i]].m_status = "published";
            }
        }
    }
    if (!saveNoLock(true))
    {
        current->second = before;
        m_sequence = oldSequence;
        if (hadPublic)
        {
            m_customRecipes[id] = oldPublic;
        }
        else
        {
            m_customRecipes.erase(id);
        }
        if (hadDraft)
        {
            m_creationDrafts[id] = oldDraft;
        }
        writePersonalizationsNoLock(personalBefore, sharedBefore);
        for (std::map<std::string, VideoInfo>::const_iterator it = mediaBefore.begin(); it != mediaBefore.end(); ++it)
        {
            m_videos[it->first] = it->second;
        }
        saveNoLock(true);
        result.addString("code", "storage_failed");
        message = "审核结果未能保存，本次未生效，请重试";
        return false;
    }
    result.addString("code", "ok");
    result.addValue("review", current->second.toJson());
    message = decision == "approved" ? "审核通过，已同步到服务器" : "不通过结果已同步到作者";
    return true;
}