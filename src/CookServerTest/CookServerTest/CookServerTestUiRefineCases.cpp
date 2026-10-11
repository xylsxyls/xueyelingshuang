#include "CookServerTestUiRefineCases.h"
#include "CookServerTestConfig.h"
#include "CStringManager/CStringManagerAPI.h"
#include <stdexcept>
#include <map>
#include <algorithm>

void CookServerTestUiRefineCases::Require(bool value, const char* message)
{
    if (!value) throw std::runtime_error(message);
}

RapidJsonDocument CookServerTestUiRefineCases::Request(CookServerTestHttpClient& client, const std::string& token,
    const std::string& path, const std::string& body, bool ok)
{
    std::map<std::string, std::string> headers;
    headers["Authorization"] = token.empty() ? "Bearer invalid" : "Bearer " + token;
    CookServerTestHttpResponse response;
    client.request(body.empty() ? "GET" : "POST", path, body, headers, response);
    RapidJsonDocument result;
    Require(response.m_networkOk && response.m_statusCode == 200, (path + " HTTP failure").c_str());
    Require(result.parse(response.m_body), "Invalid JSON response");
    Require(result.getBoolOrDefault("ok", !ok) == ok, response.m_body.c_str());
    return result;
}

RapidJsonDocument CookServerTestUiRefineCases::User(CookServerTestHttpClient& client, const std::string& suffix)
{
    return Request(client, "", "/api/auth/register", "{\"account\":\"" + g_cookServerTestConfig.m_userIdPrefix +
        "_ui_" + suffix + "\",\"password\":\"CookUiTest123!\",\"nickname\":\"Ui test\"}");
}

std::string CookServerTestUiRefineCases::Config(const std::vector<std::string>& order, const std::string& added,
    const std::vector<std::string>& disabled)
{
    RapidJsonDocument result;
    result.addString("recipeId", "cook_000002");
    result.addStringArray("taskOrder", order);
    result.addStringArray("disabledTaskIds", disabled);
    RapidJsonDocument tasks;
    tasks.parse("{\"items\":" + added + "}");
    result.addValue("addedTasks", tasks.getValue("items"));
    return result.toString();
}

void CookServerTestUiRefineCases::Run(CookServerTestHttpClient& client, const CookServerTestFeedCases::RegisterFunction& registerCase)
{
    registerCase("本人简介20字边界与多用户隔离", [&](std::string& message, int32_t& status) -> bool
    {
        try
        {
            const RapidJsonDocument a = User(client, "bio_a"), b = User(client, "bio_b");
            const std::string at = a.getStringOrDefault("token", ""), bt = b.getStringOrDefault("token", "");
            const std::string ai = a.getValue("account").getStringOrDefault("userId", ""), bi = b.getValue("account").getStringOrDefault("userId", "");
            const std::string twenty = "12345678901234567890";
            Request(client, at, "/api/users/profile/update", "{\"introduction\":\"" + twenty + "\"}");
            Require(Request(client, bt, "/api/users/profile?userId=" + ai).getValue("profile").getStringOrDefault("introduction", "") == twenty, "Public biography was not persisted");
            const char* invalid[] = {"{}", "{\"introduction\":1}", "{\"introduction\":null}", "[]", "{", "{\"introduction\":\"123456789012345678901\"}", "{\"introduction\":\"bad\\nline\"}", "{\"introduction\":\"bad\\u0000text\"}"};
            for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i) Request(client, at, "/api/users/profile/update", invalid[i], false);
            Request(client, at, "/api/users/profile/update", "{\"userId\":\"" + bi + "\",\"introduction\":\"forged\"}", false);
            Request(client, at, "/api/users/profile/update", "{\"introduction\":\"" + std::string(1, '\xC0') + "\"}", false);
            const RapidJsonValue own = Request(client, at, "/api/users/profile?userId=" + ai).getValue("profile");
            Require(own.getStringOrDefault("introduction", "") == twenty, "Rejected biography changed stored value");
            Require(Request(client, bt, "/api/users/profile?userId=" + bi).getValue("profile").getStringOrDefault("introduction", "").empty(), "Biography leaked to another user");
            Require(Request(client, at, "/api/account").getValue("account").getIntOrDefault("coins", -1) == a.getValue("account").getIntOrDefault("coins", -2), "Biography changed wallet");
            std::string chinese;
            for (size_t i = 0; i < 20; ++i) chinese += "\\u53a8";
            Request(client, at, "/api/users/profile/update", "{\"introduction\":\"" + chinese + "\"}");
            Request(client, at, "/api/users/profile/update", "{\"introduction\":\"" + chinese + "\\u53a8\"}", false);
            Request(client, at, "/api/users/profile/update", "{\"introduction\":\"\"}");
            Require(Request(client, bt, "/api/users/profile?userId=" + ai).getValue("profile").getStringOrDefault("introduction", "missing").empty(), "Empty biography did not reset");
            status = 200; return true;
        }
        catch (const std::exception& error) { message = error.what(); return false; }
    });
    registerCase("新增步骤头中尾插入删除禁用与真实排程", [&](std::string& message, int32_t& status) -> bool
    {
        try
        {
            const std::string token = User(client, "order").getStringOrDefault("token", "");
            const std::string added = "[{\"id\":\"head\",\"title\":\"Head\",\"durationSeconds\":7,\"active\":true},{\"id\":\"middle\",\"title\":\"Middle\",\"durationSeconds\":8,\"active\":true},{\"id\":\"tail\",\"title\":\"Tail\",\"durationSeconds\":9,\"active\":true}]";
            const std::vector<std::string> order = {"head", "wash_rice", "middle", "cook_rice", "fluff_rice", "tail"};
            Request(client, token, "/api/recipes/personalization/save", Config(order, added));
            const RapidJsonDocument detail = Request(client, token, "/api/recipes/personalization/detail?recipeId=cook_000002");
            Require(detail.getValue("personalization").getStringArrayOrEmpty("taskOrder") == order, "Stored order differs");
            const RapidJsonDocument plan = Request(client, token, "/api/plan/preview", "{\"recipeIds\":[\"cook_000002\"],\"summaryOnly\":false}");
            const std::vector<RapidJsonValue> timeline = plan.getArrayValueOrEmpty("timeline");
            Require(timeline.size() == order.size(), "Inserted steps missing from actual plan");
            std::map<std::string, RapidJsonValue> scheduled;
            for (size_t i = 0; i < timeline.size(); ++i)
            {
                const std::string id = timeline[i].getStringOrDefault("id", "");
                scheduled[id.substr(std::string("cook_000002.").size())] = timeline[i];
            }
            for (size_t i = 1; i < order.size(); ++i)
            {
                Require(scheduled.count(order[i]) == 1 && scheduled.count(order[i - 1]) == 1, "Plan step ID missing");
                Require(scheduled[order[i]].getIntOrDefault("startSeconds", -1) >= scheduled[order[i - 1]].getIntOrDefault("endSeconds", 99999), "Inserted order did not constrain actual cooking time");
            }
            const std::vector<std::string> disabled = {"wash_rice", "cook_rice"};
            Request(client, token, "/api/recipes/personalization/save", Config(order, added, disabled));
            Require(Request(client, token, "/api/plan/preview", "{\"recipeIds\":[\"cook_000002\"],\"summaryOnly\":false}").getArrayValueOrEmpty("timeline").size() == 4, "Disabled prerequisites were not bypassed");
            const std::vector<std::string> removed = {"wash_rice", "middle", "cook_rice", "fluff_rice"};
            Request(client, token, "/api/recipes/personalization/save", Config(removed, "[{\"id\":\"middle\",\"title\":\"Middle\",\"durationSeconds\":8,\"active\":true}]"));
            Require(Request(client, token, "/api/plan/preview", "{\"recipeIds\":[\"cook_000002\"],\"summaryOnly\":false}").getArrayValueOrEmpty("timeline").size() == 4, "Deleted inserted step survived");
            Request(client, token, "/api/recipes/personalization/reset", "{\"recipeId\":\"cook_000002\"}");
            Require(Request(client, token, "/api/plan/preview", "{\"recipeIds\":[\"cook_000002\"],\"summaryOnly\":false}").getArrayValueOrEmpty("timeline").size() == 3, "Reset failed");
            status = 200; return true;
        }
        catch (const std::exception& error) { message = error.what(); return false; }
    });
    registerCase("步骤顺序恶意及边界输入不破坏历史配置", [&](std::string& message, int32_t& status) -> bool
    {
        try
        {
            const std::string token = User(client, "invalid").getStringOrDefault("token", "");
            const std::vector<std::string> base = {"wash_rice", "cook_rice", "fluff_rice"};
            Request(client, token, "/api/recipes/personalization/save", Config(base, "[]"));
            const std::vector<std::vector<std::string>> bad = {{}, {"wash_rice", "wash_rice", "fluff_rice"}, {"wash_rice", "cook_rice"}, {"cook_rice", "wash_rice", "fluff_rice"}, {"wash_rice", "unknown", "fluff_rice"}};
            for (size_t i = 0; i < bad.size(); ++i) Request(client, token, "/api/recipes/personalization/save", Config(bad[i], "[]"), false);
            Request(client, token, "/api/recipes/personalization/save", "{\"recipeId\":\"cook_000002\",\"taskOrder\":{}}", false);
            Request(client, token, "/api/recipes/personalization/save", "{\"recipeId\":\"cook_000002\",\"taskOrder\":[1,2,3]}", false);
            const std::vector<std::string> withAdded = {"new", "wash_rice", "cook_rice", "fluff_rice"};
            Request(client, token, "/api/recipes/personalization/save", Config(withAdded, "[{\"id\":\"new\",\"title\":\"New\",\"durationSeconds\":0}]"), false);
            Request(client, token, "/api/recipes/personalization/save", Config(withAdded, "[{\"id\":\"wash_rice\",\"title\":\"New\",\"durationSeconds\":1}]"), false);
            Request(client, token, "/api/recipes/personalization/save", Config(withAdded, "[{\"id\":\"new\",\"title\":\"New\",\"durationSeconds\":1,\"dependencies\":[\"new\"]}]"), false);
            Request(client, token, "/api/recipes/personalization/save", Config(withAdded, "[{\"id\":\"new\",\"title\":\"New\",\"durationSeconds\":1,\"dependencies\":[\"missing\"]}]"), false);
            Request(client, token, "/api/recipes/personalization/save", Config(base, "[]", std::vector<std::string>(1, "unknown")), false);
            Request(client, token, "/api/recipes/personalization/save", Config(std::vector<std::string>(257, "wash_rice"), "[]"), false);
            Require(Request(client, token, "/api/recipes/personalization/detail?recipeId=cook_000002").getValue("personalization").getStringArrayOrEmpty("taskOrder") == base, "Invalid request overwrote valid order");
            std::vector<std::string> maximum = base;
            std::string many = "[";
            for (int32_t i = 0; i < 253; ++i)
            {
                const std::string id = "n" + CStringManager::toStringInt32(i);
                maximum.push_back(id);
                if (i != 0) many += ",";
                many += "{\"id\":\"" + id + "\",\"title\":\"New\",\"durationSeconds\":1,\"active\":true}";
            }
            many += "]";
            Request(client, token, "/api/recipes/personalization/save", Config(maximum, many));
            Require(Request(client, token, "/api/plan/preview", "{\"recipeIds\":[\"cook_000002\"],\"summaryOnly\":false}").getArrayValueOrEmpty("timeline").size() == 256, "Maximum valid step count failed");
            // 旧客户端没有taskOrder的配置仍按历史路径应用。
            Request(client, token, "/api/recipes/personalization/save", "{\"recipeId\":\"cook_000002\",\"durationOverrides\":[{\"taskId\":\"wash_rice\",\"durationSeconds\":30}]}" );
            Require(Request(client, token, "/api/plan/preview", "{\"recipeIds\":[\"cook_000002\"],\"summaryOnly\":false}").getArrayValueOrEmpty("timeline").size() == 3, "Legacy configuration failed");
            status = 200; return true;
        }
        catch (const std::exception& error) { message = error.what(); return false; }
    });
    registerCase("首页原视频同作者及三用户真实互动状态", [&](std::string& message, int32_t& status) -> bool
    {
        try
        {
            const std::string at = User(client, "social_a").getStringOrDefault("token", ""), bt = User(client, "social_b").getStringOrDefault("token", ""), ct = User(client, "social_c").getStringOrDefault("token", "");
            const std::vector<RapidJsonValue> feed = Request(client, at, "/api/feed?count=30").getArrayValueOrEmpty("feed");
            RapidJsonValue original;
            for (size_t i = 0; i < feed.size(); ++i) if (feed[i].getStringOrDefault("authorUserId", "") == "qmzdc") { original = feed[i]; break; }
            const std::string video = original.getStringOrDefault("videoId", "");
            Require(!video.empty(), "No system video fixture");
            const std::string url = "/api/videos/detail?videoId=" + video;
            RapidJsonValue detail = Request(client, at, url).getValue("item");
            Require(detail.getStringOrDefault("author", "") == original.getStringOrDefault("author", "") && detail.getStringOrDefault("authorAvatarUrl", "") == original.getStringOrDefault("authorAvatarUrl", ""), "Home and detail author differ");
            const int32_t before = detail.getIntOrDefault("likes", -1);
            const std::string body = "{\"targetType\":\"video\",\"targetId\":\"" + video + "\"}";
            const RapidJsonDocument liked = Request(client, at, "/api/likes/toggle", body);
            Require(liked.getBoolOrDefault("liked", false) && liked.getIntOrDefault("likes", -1) == before + 1, "Like response did not contain actual count");
            Request(client, bt, "/api/likes/toggle", body);
            const RapidJsonValue third = Request(client, ct, url).getValue("item");
            Require(!third.getBoolOrDefault("liked", true) && third.getIntOrDefault("likes", -1) == before + 2, "Global count or user-specific like leaked");
            Request(client, at, "/api/follows/toggle", "{\"targetUserId\":\"qmzdc\"}");
            Require(Request(client, at, url).getValue("item").getBoolOrDefault("authorFollowing", false), "Following missing in video metadata");
            Require(!Request(client, ct, url).getValue("item").getBoolOrDefault("authorFollowing", true), "Following leaked to third user");
            Request(client, at, "/api/favorites/toggle", body);
            Require(Request(client, at, url).getValue("item").getBoolOrDefault("favorite", false), "Favorite missing");
            Require(!Request(client, bt, url).getValue("item").getBoolOrDefault("favorite", true), "Favorite leaked");
            Request(client, at, "/api/likes/toggle", body);
            Require(Request(client, ct, url).getValue("item").getIntOrDefault("likes", -1) == before + 1, "Unlike count wrong");
            status = 200; return true;
        }
        catch (const std::exception& error) { message = error.what(); return false; }
    });
}