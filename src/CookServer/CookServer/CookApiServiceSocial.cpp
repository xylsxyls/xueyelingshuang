#include "CookApiService.h"
#include "CookHelper.h"
#include "CookServerHelper.h"
#include "CStringManager/CStringManagerAPI.h"
#include "RapidJson/RapidJsonAPI.h"
#include <algorithm>

RapidJsonValue CookApiService::publicProfileToJson(const UserContactInfo& info) const
{
    RapidJsonValue object;
    object.setObject();
    CookServerHelper::addString(object, "userId", info.m_userId);
    CookServerHelper::addString(object, "nickname", info.m_nickname);
    CookServerHelper::addString(object, "avatarUrl", info.m_avatarUrl);
    CookServerHelper::addString(object, "introduction", info.m_introduction);
    object.addBool("following", info.m_following);
    object.addBool("friend", info.m_friend);
    object.addInt("followerCount", info.m_followerCount);
    object.addInt("receivedLikeCount", info.m_receivedLikeCount);
    return object;
}

std::string CookApiService::buildSocialJson(const HttpRequest& request, const std::string& userId)
{
    RapidJsonDocument document;
    document.setObject();
    if (request.m_uri == "/api/users/profile/update")
    {
        RapidJsonDocument body;
        const std::string missing(1, '\x01');
        const bool parsed = body.parse(request.m_body) && body.isObject() && body.getStringOrDefault("introduction", missing) != missing;
        const std::string target = parsed ? body.getStringOrDefault("userId", userId) : "";
        std::string message;
        const bool ok = parsed && target == userId && m_accountStore.updateIntroduction(userId, body.getStringOrDefault("introduction", ""), message);
        document.addBool("ok", ok);
        CookServerHelper::addString(document, "message", ok ? "ok" : (message.empty() ? "Only your own introduction can be updated" : message));
        if (ok) { UserContactInfo profile; m_accountStore.getPublicProfile(userId, userId, profile); document.addValue("profile", publicProfileToJson(profile)); }
        return document.toString();
    }
    if (request.m_uri == "/api/recipes/menu/remove")
    {
        std::string message;
        const bool ok = m_accountStore.removeMenuRecipe(userId, jsonFieldString(request.m_body, "recipeId"), message);
        document.addBool("ok", ok);
        CookServerHelper::addString(document, "message", message);
        return document.toString();
    }
    const std::string targetId = request.m_uri == "/api/messages/read" ? jsonFieldString(request.m_body, "targetUserId") : CookHelper::queryValue(request, "userId");
    if (request.m_uri == "/api/users/fans")
    {
        if (!targetId.empty() && targetId != userId)
        {
            document.addBool("ok", false);
            CookServerHelper::addString(document, "message", "Only your own fans list is available");
            return document.toString();
        }
        const std::vector<UserContactInfo> followers = m_accountStore.listFollowers(userId);
        const std::string cursor = CookHelper::queryValue(request, "cursor");
        const int32_t count = std::max(1, std::min(60, CookHelper::queryValue(request, "count").empty() ? 30 : static_cast<int32_t>(CStringManager::atoi64(CookHelper::queryValue(request, "count").c_str()))));
        RapidJsonValue list;
        list.setArray();
        std::string next;
        bool hasMore = false;
        for (size_t i = 0; i < followers.size(); ++i)
        {
            if (followers[i].m_userId <= cursor) continue;
            if (list.size() >= static_cast<size_t>(count)) { hasMore = true; break; }
            list.pushValue(publicProfileToJson(followers[i]));
            next = followers[i].m_userId;
        }
        document.addBool("ok", true);
        document.addBool("hasMore", hasMore);
        CookServerHelper::addString(document, "cursor", next);
        document.addValue("users", list);
        return document.toString();
    }
    if (request.m_uri == "/api/videos/detail")
    {
        VideoInfo video;
        std::string path, contentType;
        const bool ok = m_accountStore.getVideoFile(CookHelper::queryValue(request, "videoId"), video, path, contentType, userId) && (video.m_mediaType == "video" || video.m_mediaType == "image");
        document.addBool("ok", ok);
        if (ok)
        {
            const UserAccount viewer = m_accountStore.getAccount(userId);
            std::map<std::string, Recipe> recipes;
            for (size_t i = 0; i < video.m_recipeIds.size(); ++i)
            {
                Recipe recipe;
                std::string message;
                if (m_accountStore.getRecipeForUser(userId, video.m_recipeIds[i], false, recipe, message)) recipes[recipe.m_id] = recipe;
            }
            document.addValue("item", feedVideoToJson(request, video, viewer, recipes, m_accountStore.getUserDisplayName(video.m_ownerUserId), 0));
        }
        else CookServerHelper::addString(document, "message", "Video is unavailable");
        return document.toString();
    }
    UserContactInfo profile;
    if (targetId.empty() || !m_accountStore.getPublicProfile(userId, targetId, profile))
    {
        document.addBool("ok", false);
        CookServerHelper::addString(document, "message", "User not found");
        return document.toString();
    }
    if (request.m_uri == "/api/users/feed")
    {
        const std::string mode = CookHelper::queryValue(request, "mode");
        if (mode != "created" && mode != "liked")
        {
            document.addBool("ok", false);
            CookServerHelper::addString(document, "message", "Only created and liked videos are public");
            return document.toString();
        }
        const int32_t count = std::max(1, std::min(30, CookHelper::queryValue(request, "count").empty() ? 18 : static_cast<int32_t>(CStringManager::atoi64(CookHelper::queryValue(request, "count").c_str()))));
        const std::vector<std::string> exclusions = CookServerHelper::split(CookHelper::queryValue(request, "excludeIds"), ',');
        const std::set<std::string> exclude(exclusions.begin(), exclusions.end());
        bool reset = false;
        const std::vector<VideoInfo> videos = m_accountStore.listFeedVideosForUser(targetId, count + 1, exclude, mode, "", reset);
        const UserAccount viewer = m_accountStore.getAccount(userId);
        std::map<std::string, Recipe> recipes;
        const std::vector<Recipe> visibleRecipes = m_accountStore.listRecipesForUser(userId);
        for (size_t i = 0; i < visibleRecipes.size(); ++i) recipes[visibleRecipes[i].m_id] = visibleRecipes[i];
        RapidJsonValue list;
        list.setArray();
        for (size_t i = 0; i < videos.size() && i < static_cast<size_t>(count); ++i)
            list.pushValue(feedVideoToJson(request, videos[i], viewer, recipes, m_accountStore.getUserDisplayName(videos[i].m_ownerUserId), static_cast<int32_t>(i)));
        document.addBool("ok", true);
        document.addBool("hasMore", videos.size() > static_cast<size_t>(count));
        document.addValue("items", list);
        return document.toString();
    }
    if (request.m_uri == "/api/messages/read")
    {
        m_accountStore.readConversation(userId, targetId);
        document.addBool("ok", true);
        return document.toString();
    }
    if (request.m_uri == "/api/messages/conversation")
    {
        const std::vector<MessageInfo> messages = m_accountStore.listConversation(userId, targetId);
        const std::string before = CookHelper::queryValue(request, "beforeId");
        size_t end = messages.size();
        if (!before.empty())
        {
            end = 0;
            while (end < messages.size() && messages[end].m_id != before) ++end;
        }
        const size_t start = end > 60 ? end - 60 : 0;
        RapidJsonValue list;
        list.setArray();
        for (size_t i = start; i < end; ++i) list.pushValue(messageToJson(messages[i]));
        document.addBool("ok", true);
        document.addBool("hasMore", start > 0);
        document.addBool("muted", !profile.m_friend);
        document.addValue("messages", list);
        document.addValue("profile", publicProfileToJson(profile));
        return document.toString();
    }
    document.addBool("ok", true);
    document.addBool("self", targetId == userId);
    document.addValue("profile", publicProfileToJson(profile));
    return document.toString();
}