#include "CookServerTestFeedCases.h"
#include "CookServerTestConfig.h"
#include "RapidJson/RapidJsonAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include <set>

bool CookServerTestFeedCases::check(const CookServerTestHttpResponse& response, int32_t expectedStatus,
                                   bool expectedOk, std::string& message, int32_t& statusCode)
{
    statusCode = response.m_statusCode;
    RapidJsonDocument document;
    if (!response.m_networkOk || statusCode != expectedStatus || !document.parse(response.m_body) ||
        !document.isObject() || document.getBoolOrDefault("ok", !expectedOk) != expectedOk)
    {
        message = "unexpected feed response or network failure: " + response.m_error;
        return false;
    }
    return true;
}

bool CookServerTestFeedCases::expectFeed(CookServerTestHttpClient& client, const std::string& token,
                                        const std::string& query, const std::vector<std::string>& expectedIds,
                                        const std::string& owner, std::string& message, int32_t& statusCode)
{
    std::map<std::string, std::string> headers;
    headers["Authorization"] = "Bearer " + token;
    CookServerTestHttpResponse response;
    client.get("/api/feed?" + query, headers, response);
    if (!check(response, 200, true, message, statusCode)) return false;
    RapidJsonDocument document;
    document.parse(response.m_body);
    const std::vector<RapidJsonValue> feed = document.getArrayValueOrEmpty("feed");
    std::set<std::string> actual;
    for (size_t i = 0; i < feed.size(); ++i)
    {
        const std::string id = feed[i].getStringOrDefault("videoId", "");
        if (id.empty() || !actual.insert(id).second ||
            (!owner.empty() && feed[i].getStringOrDefault("authorUserId", "") != owner) ||
            feed[i].getStringOrDefault("posterUrl", "").empty())
        { message = "duplicate ID, missing poster or foreign author in personal feed"; return false; }
    }
    if (actual != std::set<std::string>(expectedIds.begin(), expectedIds.end()) ||
        document.getBoolOrDefault("resetSeen", true))
    { message = "personal feed differs from expected IDs or reset watched history: " + query; return false; }
    return true;
}

bool CookServerTestFeedCases::toggle(CookServerTestHttpClient& client, const std::string& token,
                                    const std::string& type, const std::string& id, bool expected,
                                    std::string& message, int32_t& statusCode)
{
    RapidJsonDocument body;
    body.setObject();
    body.addString("targetType", type);
    body.addString("targetId", id);
    std::map<std::string, std::string> headers;
    headers["Authorization"] = "Bearer " + token;
    CookServerTestHttpResponse response;
    client.post("/api/favorites/toggle", body.toString(), headers, response);
    if (!check(response, 200, true, message, statusCode)) return false;
    RapidJsonDocument document;
    document.parse(response.m_body);
    if (document.getBoolOrDefault("favorite", !expected) != expected)
    { message = "favorite toggle state differs from expected"; return false; }
    return true;
}

void CookServerTestFeedCases::run(CookServerTestHttpClient& client, const std::string& token,
                                 const std::string& createdVideoId, const RegisterFunction& registerCase)
{
    std::string peerToken;
    std::string owner;
    std::string recipeVideoId;
    std::string recipeId;
    const std::vector<std::string> empty;
    registerCase("本人已发布作品与会话隔离", [&](std::string& message, int32_t& statusCode) -> bool
    {
        if (createdVideoId.empty()) { message = "published upload fixture missing"; return false; }
        std::map<std::string, std::string> headers;
        headers["Authorization"] = "Bearer " + token;
        CookServerTestHttpResponse session;
        client.get("/api/auth/session", headers, session);
        if (!check(session, 200, true, message, statusCode)) return false;
        RapidJsonDocument account;
        account.parse(session.m_body);
        owner = account.getValue("account").getStringOrDefault("userId", "");
        if (owner.empty()) { message = "missing authenticated owner"; return false; }
        std::vector<std::string> created(1, createdVideoId);
        if (!expectFeed(client, token, "mode=created&count=60", created, owner, message, statusCode)) return false;
        RapidJsonDocument body;
        body.setObject();
        body.addString("account", g_cookServerTestConfig.m_userIdPrefix + "_feedother");
        body.addString("password", "cook1234");
        body.addString("nickname", "feed peer");
        CookServerTestHttpResponse registration;
        client.post("/api/auth/register", body.toString(), registration);
        if (!check(registration, 200, true, message, statusCode)) return false;
        RapidJsonDocument peer;
        peer.parse(registration.m_body);
        peerToken = peer.getStringOrDefault("token", "");
        if (peerToken.empty()) { message = "missing peer token"; return false; }
        if (!expectFeed(client, peerToken, "mode=created&count=60&userId=" + CStringManager::UrlEncode(owner),
                        empty, "", message, statusCode)) return false;
        headers["Authorization"] = "Bearer invalid_feed_token";
        CookServerTestHttpResponse invalid;
        client.get("/api/feed?mode=created", headers, invalid);
        return check(invalid, 401, false, message, statusCode);
    });
    registerCase("视频与菜谱收藏合并去重及独立取消", [&](std::string& message, int32_t& statusCode) -> bool
    {
        if (peerToken.empty()) { message = "peer registration failed"; return false; }
        std::map<std::string, std::string> headers;
        headers["Authorization"] = "Bearer " + peerToken;
        CookServerTestHttpResponse recommendations;
        client.get("/api/feed?count=60", headers, recommendations);
        if (!check(recommendations, 200, true, message, statusCode)) return false;
        RapidJsonDocument document;
        document.parse(recommendations.m_body);
        const std::vector<RapidJsonValue> feed = document.getArrayValueOrEmpty("feed");
        for (size_t i = 0; i < feed.size(); ++i)
        {
            const std::vector<std::string> ids = feed[i].getStringArrayOrEmpty("recipeIds");
            if (!ids.empty()) { recipeId = ids[0]; recipeVideoId = feed[i].getStringOrDefault("videoId", ""); break; }
        }
        if (recipeId.empty() || recipeVideoId.empty()) { message = "missing recipe-linked media fixture"; return false; }
        std::vector<std::string> expected(1, recipeVideoId);
        if (!expectFeed(client, peerToken, "mode=collection&count=60", empty, "", message, statusCode)) return false;
        if (!toggle(client, peerToken, "recipe", recipeId, true, message, statusCode) ||
            !expectFeed(client, peerToken, "mode=collection&count=60", expected, "", message, statusCode) ||
            !expectFeed(client, peerToken, "mode=favorite&count=60", empty, "", message, statusCode)) return false;
        if (!toggle(client, peerToken, "video", recipeVideoId, true, message, statusCode) ||
            !expectFeed(client, peerToken, "mode=collection&count=60", expected, "", message, statusCode) ||
            !expectFeed(client, peerToken, "mode=favorite&count=60", expected, "", message, statusCode)) return false;
        if (!toggle(client, peerToken, "recipe", recipeId, false, message, statusCode) ||
            !expectFeed(client, peerToken, "mode=collection&count=60", expected, "", message, statusCode) ||
            !toggle(client, peerToken, "video", recipeVideoId, false, message, statusCode) ||
            !expectFeed(client, peerToken, "mode=collection&count=60", empty, "", message, statusCode)) return false;
        // 逆序取消：取消视频不能删除仍由菜谱收藏保留的封面。
        if (!toggle(client, peerToken, "recipe", recipeId, true, message, statusCode) ||
            !toggle(client, peerToken, "video", recipeVideoId, true, message, statusCode) ||
            !toggle(client, peerToken, "video", recipeVideoId, false, message, statusCode) ||
            !expectFeed(client, peerToken, "mode=collection&count=60", expected, "", message, statusCode)) return false;
        return toggle(client, peerToken, "recipe", recipeId, false, message, statusCode);
    });
    registerCase("个人视频分页排除与看过状态边界", [&](std::string& message, int32_t& statusCode) -> bool
    {
        if (peerToken.empty() || recipeVideoId.empty()) { message = "personal feed fixtures missing"; return false; }
        if (!toggle(client, peerToken, "video", recipeVideoId, true, message, statusCode)) return false;
        std::vector<std::string> expected(1, recipeVideoId);
        if (!expectFeed(client, peerToken, "mode=collection&count=1", expected, "", message, statusCode) ||
            !expectFeed(client, peerToken, "mode=collection&count=1&excludeIds=" +
                CStringManager::UrlEncode("feed_" + recipeVideoId + "," + recipeVideoId + "," + recipeVideoId),
                empty, "", message, statusCode) ||
            !expectFeed(client, token, "mode=created&count=1&excludeIds=" + CStringManager::UrlEncode("feed_" + createdVideoId),
                empty, owner, message, statusCode)) return false;
        std::map<std::string, std::string> headers;
        headers["Authorization"] = "Bearer " + peerToken;
        RapidJsonDocument watch;
        watch.setObject();
        watch.addString("videoId", recipeVideoId);
        CookServerTestHttpResponse response;
        client.post("/api/feed/watch", watch.toString(), headers, response);
        if (!check(response, 200, true, message, statusCode) ||
            !expectFeed(client, peerToken, "mode=collection&count=1", expected, "", message, statusCode)) return false;
        client.get("/api/account", headers, response);
        if (!check(response, 200, true, message, statusCode)) return false;
        RapidJsonDocument account;
        account.parse(response.m_body);
        if (account.getValue("account").getStringArrayOrEmpty("watchedVideoIds").empty())
        { message = "collection unexpectedly cleared watched state"; return false; }
        return toggle(client, peerToken, "video", recipeVideoId, false, message, statusCode);
    });
}