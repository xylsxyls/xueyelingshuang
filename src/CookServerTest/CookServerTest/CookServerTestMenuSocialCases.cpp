#include "CookServerTestMenuSocialCases.h"
#include "CookServerTestConfig.h"
#include "CStringManager/CStringManagerAPI.h"
#include <stdexcept>
#include <set>

void CookServerTestMenuSocialCases::Require(bool valid, const char* message)
{
    if (!valid) throw std::runtime_error(message);
}

RapidJsonDocument CookServerTestMenuSocialCases::Request(CookServerTestHttpClient& client, const std::string& token,
    const std::string& path, const std::string& body, bool expectedOk, int32_t status)
{
    CookServerTestHttpResponse response;
    std::map<std::string, std::string> headers;
    // Explicit empty Authorization avoids inheriting the runner's authenticated user.
    headers["Authorization"] = token.empty() ? "Bearer invalid" : "Bearer " + token;
    client.request(body.empty() ? "GET" : "POST", path, body, headers, response);
    RapidJsonDocument document;
    if (!response.m_networkOk || response.m_statusCode != status) throw std::runtime_error(path + " HTTP " + CStringManager::toStringInt32(response.m_statusCode) + " " + response.m_body);
    Require(document.parse(response.m_body) && document.isObject(), "Invalid response JSON");
    Require(document.getBoolOrDefault("ok", !expectedOk) == expectedOk, response.m_body.c_str());
    return document;
}

RapidJsonDocument CookServerTestMenuSocialCases::NewUser(CookServerTestHttpClient& client, const std::string& suffix)
{
    const std::string account = g_cookServerTestConfig.m_userIdPrefix + "_ms_" + suffix;
    return Request(client, "", "/api/auth/register", "{\"account\":\"" + account + "\",\"password\":\"cookTest123\",\"nickname\":\"Menu test\"}");
}

RapidJsonDocument CookServerTestMenuSocialCases::Recipe(CookServerTestHttpClient& client, const std::string& token,
    const std::string& id, const std::string& title, const std::string& grams, bool expectedOk)
{
    return Request(client, token, "/api/recipes/custom/publish", "{\"recipe\":{\"schemaVersion\":\"1.0\",\"id\":\"" + id +
        "\",\"title\":\"Menu fixture\",\"category\":\"\\u852c\\u83dc\",\"priceType\":\"free\",\"ingredients\":[{\"name\":\"Potato\",\"amount\":\"" + grams +
        "\"}],\"tasks\":[{\"id\":\"cut\",\"title\":\"" + title + "\",\"detail\":\"Do not speak this\",\"durationSeconds\":10,\"active\":true}]}}", expectedOk);
}

void CookServerTestMenuSocialCases::Run(CookServerTestHttpClient& client, const std::string& fixtureVideoId,
    const CookServerTestFeedCases::RegisterFunction& registerCase)
{
    registerCase("菜单加入与购买分离及幂等", [&](std::string& message, int32_t& status) -> bool
    {
        try
        {
            const RapidJsonDocument user = NewUser(client, "paid");
            const std::string token = user.getStringOrDefault("token", "");
            const int32_t coins = user.getValue("account").getIntOrDefault("coins", -1);
            const RapidJsonDocument before = Request(client, token, "/api/recipes");
            const std::vector<RapidJsonValue> initial = before.getArrayValueOrEmpty("recipes");
            for (size_t i = 0; i < initial.size(); ++i) Require(initial[i].getStringOrDefault("id", "") != "cook_000006", "unjoined paid recipe leaked into menu");
            const std::string body = "{\"recipeId\":\"cook_000006\",\"idempotencyKey\":\"same\"}";
            const RapidJsonDocument joined = Request(client, token, "/api/recipes/join", body);
            Require(joined.getValue("account").getIntOrDefault("coins", -2) == coins, "join charged coins");
            Require(!joined.getValue("recipe").getBoolOrDefault("accessible", true) && joined.getValue("recipe").getBoolOrDefault("inMenu", false), "paid join granted permission or not in menu");
            Request(client, token, "/api/recipes/join", body);
            const RapidJsonDocument bought = Request(client, token, "/api/recipes/purchase", body);
            const RapidJsonDocument twice = Request(client, token, "/api/recipes/purchase", body);
            Require(bought.getValue("recipe").getBoolOrDefault("purchased", false) && bought.getValue("recipe").getBoolOrDefault("accessible", false), "purchase did not grant access");
            Require(bought.getValue("account").getIntOrDefault("coins", -1) == coins - 18 && twice.getValue("account").getIntOrDefault("coins", -2) == coins - 18, "purchase charged incorrectly");
            status = 200; return true;
        } catch (const std::exception& error) { message = error.what(); return false; }
    });
    registerCase("菜单移除确认后状态及查看排序", [&](std::string& message, int32_t& status) -> bool
    {
        try
        {
            const std::string token = NewUser(client, "order").getStringOrDefault("token", "");
            const std::string nonce = g_cookServerTestConfig.m_userIdPrefix + "_free";
            const std::string id = Recipe(client, token, nonce).getValue("recipe").getStringOrDefault("id", "");
            Request(client, token, "/api/recipes/join", "{\"recipeId\":\"" + id + "\"}");
            Request(client, token, "/api/recipes/join", "{\"recipeId\":\"cook_000006\"}");
            Request(client, token, "/api/recipes/detail?recipeId=" + id);
            RapidJsonDocument menu = Request(client, token, "/api/recipes");
            Require(!menu.getArrayValueOrEmpty("recipes").empty() && menu.getArrayValueOrEmpty("recipes")[0].getStringOrDefault("id", "") == id, "detail did not move recipe to top");
            Request(client, token, "/api/recipes/menu/remove", "{\"recipeId\":\"" + id + "\"}");
            Request(client, token, "/api/recipes/menu/remove", "{\"recipeId\":\"" + id + "\"}");
            menu = Request(client, token, "/api/recipes");
            const std::vector<RapidJsonValue> list = menu.getArrayValueOrEmpty("recipes");
            for (size_t i = 0; i < list.size(); ++i) Require(list[i].getStringOrDefault("id", "") != id, "removed recipe was re-added");
            Request(client, token, "/api/recipes/purchase", "{\"recipeId\":\"cook_000006\"}");
            Request(client, token, "/api/recipes/menu/remove", "{\"recipeId\":\"cook_000006\"}", false);
            status = 200; return true;
        } catch (const std::exception& error) { message = error.what(); return false; }
    });
    registerCase("公开资料隐私与媒体模式边界", [&](std::string& message, int32_t& status) -> bool
    {
        try
        {
            const RapidJsonDocument user = NewUser(client, "public");
            const std::string token = user.getStringOrDefault("token", "");
            const std::string id = user.getValue("account").getStringOrDefault("userId", "");
            const RapidJsonDocument profile = Request(client, token, "/api/users/profile?userId=" + id);
            const std::string json = profile.getValue("profile").toString();
            const char* forbidden[] = { "coins", "yuanbao", "password", "favorite", "ownedRecipe", "menuRecipe", "account\"" };
            for (size_t i = 0; i < 7; ++i) Require(json.find(forbidden[i]) == std::string::npos, "private account fields exposed");
            Request(client, token, "/api/users/feed?userId=" + id + "&mode=created&count=18");
            Request(client, token, "/api/users/feed?userId=" + id + "&mode=liked&count=18");
            Request(client, token, "/api/users/feed?userId=" + id + "&mode=favorite", "", false);
            Request(client, token, "/api/users/profile?userId=missing_public_user", "", false);
            Request(client, "", "/api/users/profile?userId=" + id, "", false, 401);
            status = 200; return true;
        } catch (const std::exception& error) { message = error.what(); return false; }
    });
    registerCase("粉丝互关好友与取消关注", [&](std::string& message, int32_t& status) -> bool
    {
        try
        {
            const RapidJsonDocument a = NewUser(client, "fans_a"), b = NewUser(client, "fans_b");
            const std::string at = a.getStringOrDefault("token", ""), bt = b.getStringOrDefault("token", "");
            const std::string ai = a.getValue("account").getStringOrDefault("userId", ""), bi = b.getValue("account").getStringOrDefault("userId", "");
            Request(client, at, "/api/follows/toggle", "{\"targetUserId\":\"" + bi + "\"}");
            Require(Request(client, bt, "/api/users/fans?count=30").getArrayValueOrEmpty("users").size() == 1, "missing fan");
            Request(client, at, "/api/users/fans?userId=" + bi, "", false);
            Request(client, bt, "/api/follows/toggle", "{\"targetUserId\":\"" + ai + "\"}");
            Require(Request(client, at, "/api/users/profile?userId=" + bi).getValue("profile").getBoolOrDefault("friend", false), "mutual follow did not create friendship");
            Request(client, bt, "/api/follows/toggle", "{\"targetUserId\":\"" + ai + "\"}");
            Require(!Request(client, at, "/api/users/profile?userId=" + bi).getValue("profile").getBoolOrDefault("friend", true), "unfollow left stale friendship");
            status = 200; return true;
        } catch (const std::exception& error) { message = error.what(); return false; }
    });
    registerCase("私信免打扰互关已读与会话隔离", [&](std::string& message, int32_t& status) -> bool
    {
        try
        {
            const RapidJsonDocument a = NewUser(client, "chat_a"), b = NewUser(client, "chat_b"), c = NewUser(client, "chat_c");
            const std::string at = a.getStringOrDefault("token", ""), bt = b.getStringOrDefault("token", ""), ct = c.getStringOrDefault("token", "");
            const std::string ai = a.getValue("account").getStringOrDefault("userId", ""), bi = b.getValue("account").getStringOrDefault("userId", "");
            const std::string send = "{\"targetUserId\":\"" + bi + "\",\"text\":\"hello\"}";
            Request(client, at, "/api/messages/send", send, false);
            Request(client, at, "/api/follows/toggle", "{\"targetUserId\":\"" + bi + "\"}");
            Request(client, at, "/api/messages/send", send);
            RapidJsonDocument inbox = Request(client, bt, "/api/messages/list");
            bool muted = false;
            const std::vector<RapidJsonValue> list = inbox.getArrayValueOrEmpty("messages");
            for (size_t i = 0; i < list.size(); ++i) if (list[i].getStringOrDefault("type", "") == "chat") muted = list[i].getBoolOrDefault("muted", false) && !list[i].getBoolOrDefault("read", true);
            Require(muted, "one-way message not muted/unread");
            Require(Request(client, ct, "/api/messages/conversation?userId=" + ai).getArrayValueOrEmpty("messages").empty(), "foreign conversation leaked");
            Request(client, bt, "/api/follows/toggle", "{\"targetUserId\":\"" + ai + "\"}");
            Require(!Request(client, bt, "/api/messages/conversation?userId=" + ai).getBoolOrDefault("muted", true), "friend conversation still muted");
            Request(client, bt, "/api/messages/read", "{\"targetUserId\":\"" + ai + "\"}");
            Require(Request(client, bt, "/api/messages/conversation?userId=" + ai).getArrayValueOrEmpty("messages")[0].getBoolOrDefault("read", false), "read marker not persisted in store");
            Request(client, bt, "/api/messages/send", "{\"targetUserId\":\"" + ai + "\",\"text\":\"reply\"}");
            Require(Request(client, at, "/api/messages/conversation?userId=" + bi).getArrayValueOrEmpty("messages").size() == 2, "missing sent or received history");
            status = 200; return true;
        } catch (const std::exception& error) { message = error.what(); return false; }
    });
    registerCase("真实视频详情及原视频封面引用", [&](std::string& message, int32_t& status) -> bool
    {
        try
        {
            const std::string token = NewUser(client, "media").getStringOrDefault("token", "");
            const RapidJsonDocument video = Request(client, token, "/api/videos/detail?videoId=" + fixtureVideoId);
            const RapidJsonValue item = video.getValue("item");
            Require(item.getStringOrDefault("videoId", "") == fixtureVideoId && !item.getStringOrDefault("authorUserId", "").empty(), "original video metadata incomplete");
            Require(!item.getStringOrDefault("videoUrl", "").empty() && !item.getStringOrDefault("posterUrl", "").empty(), "missing video/poster URLs");
            const RapidJsonValue recipe = Request(client, token, "/api/recipes/detail?recipeId=cook_000001").getValue("recipe");
            Require(!recipe.getStringOrDefault("videoId", "").empty() && !recipe.getStringOrDefault("posterUrl", "").empty(), "recipe missing original video");
            Require(recipe.toString().find("coverColor") == std::string::npos, "colored fake recipe cover retained");
            Request(client, token, "/api/videos/detail?videoId=missing_video", "", false);
            status = 200; return true;
        } catch (const std::exception& error) { message = error.what(); return false; }
    });
    registerCase("全部内置菜克数短标题与仅标题语音", [&](std::string& message, int32_t& status) -> bool
    {
        try
        {
            const std::string token = NewUser(client, "short").getStringOrDefault("token", "");
            Request(client, token, "/api/recipes/purchase", "{\"recipeId\":\"cook_000006\"}");
            Request(client, token, "/api/recipes/purchase", "{\"recipeId\":\"cook_000007\"}");
            const std::vector<RapidJsonValue> recipes = Request(client, token, "/api/recipes?scope=catalog").getArrayValueOrEmpty("recipes");
            int32_t checked = 0;
            for (size_t i = 0; i < recipes.size(); ++i)
            {
                const std::string id = recipes[i].getStringOrDefault("id", "");
                if (id.find("cook_00000") != 0) continue;
                const RapidJsonValue recipe = Request(client, token, "/api/recipes/detail?recipeId=" + id).getValue("recipe");
                const std::vector<RapidJsonValue> ingredients = recipe.getArrayValueOrEmpty("ingredients"), tasks = recipe.getArrayValueOrEmpty("tasks");
                for (size_t j = 0; j < ingredients.size(); ++j)
                {
                    const std::string amount = ingredients[j].getStringOrDefault("amount", "");
                    Require(amount.size() > 3 && amount.substr(amount.size() - 3) == "\xE5\x85\x8B", "ingredient not in grams");
                }
                Require(!tasks.empty(), "builtin steps missing");
                for (size_t j = 0; j < tasks.size(); ++j)
                {
                    const std::string title = tasks[j].getStringOrDefault("title", "");
                    size_t length = 0;
                    for (size_t k = 0; k < title.size(); ++k) if ((static_cast<unsigned char>(title[k]) & 0xC0) != 0x80) ++length;
                    Require(length > 0 && length <= 8 && tasks[j].getStringOrDefault("voiceText", "") == title, "step title/voice contract violated");
                }
                ++checked;
            }
            Require(checked == 7, "did not check all builtins");
            status = 200; return true;
        } catch (const std::exception& error) { message = error.what(); return false; }
    });
    registerCase("新菜谱输入封面所有权与图片上传边界", [&](std::string& message, int32_t& status) -> bool
    {
        try
        {
            const std::string token = NewUser(client, "input").getStringOrDefault("token", "");
            const std::string nonce = g_cookServerTestConfig.m_userIdPrefix + "_input";
            const std::string id = Recipe(client, token, nonce, "12345678").getValue("recipe").getStringOrDefault("id", "");
            Recipe(client, token, id, "123456789", "100\\u514b", false);
            Recipe(client, token, id, "Cut", "1 spoon", false);
            Recipe(client, token, id, "Cut", "0\\u514b", false);
            Request(client, token, "/api/videos/upload/start", "{\"mediaType\":\"image\",\"fileName\":\"x.png\",\"fileSizeBytes\":4194305}", false);
            Request(client, token, "/api/videos/upload/start", "{\"mediaType\":\"image\",\"fileName\":\"x.exe\",\"fileSizeBytes\":20}", false);
            Request(client, token, "/api/videos/publish", "{\"videoId\":\"" + fixtureVideoId + "\",\"recipeIds\":[\"" + id + "\"],\"coverImageId\":\"missing\"}", false);
            const RapidJsonValue recipe = Request(client, token, "/api/recipes/detail?recipeId=" + id).getValue("recipe");
            Require(!recipe.getArrayValueOrEmpty("tasks").empty() && recipe.getArrayValueOrEmpty("tasks")[0].getStringOrDefault("title", "") == "12345678", "failed update mutated stored recipe");
            status = 200; return true;
        } catch (const std::exception& error) { message = error.what(); return false; }
    });
}