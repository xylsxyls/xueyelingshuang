#include "CookServerTestReviewCases.h"
#include "CookServerTestConfig.h"
#include "CookServerTestStressTask.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <atomic>
#include <fstream>
#include <memory>

bool CookServerTestReviewCases::call(CookServerTestHttpClient &client, const std::string &token,
    const std::string &route, const std::string &body, RapidJsonDocument &result, std::string &message, bool expectedOk)
{
    std::map<std::string, std::string> headers;
    headers["Authorization"] = "Bearer " + token;
    CookServerTestHttpResponse response;
    client.request(body.empty() ? "GET" : "POST", route, body, headers, response);
    return check(response.m_networkOk && response.m_statusCode == 200 && result.parse(response.m_body) &&
                     result.getBoolOrDefault("ok", !expectedOk) == expectedOk,
        "Review response differs: " + response.m_body, message);
}

bool CookServerTestReviewCases::check(bool condition, const std::string &detail, std::string &message)
{
    if (!condition)
    {
        message = detail;
    }
    return condition;
}

std::string CookServerTestReviewCases::publication(const std::string &id, const std::string &price)
{
    return "{\"recipe\":{\"schemaVersion\":\"1.0\",\"id\":\"" + id +
           "\",\"title\":\"Review fixture\",\"category\":\"vegetable\",\"priceType\":\"" + price +
           "\",\"priceAmount\":" + (price == "free" ? "0" : "7") +
           ",\"ingredients\":[{\"name\":\"Potato\",\"amount\":\"100\\u514b\"}],\"tasks\":[{\"id\":\"cut\",\"title\":"
           "\"Cut\",\"detail\":\"Cut carefully\",\"durationSeconds\":10,\"active\":true}]}}";
}

bool CookServerTestReviewCases::fixture(CookServerTestHttpClient &client, const std::string &name, std::string &token,
    std::string &admin, RapidJsonValue &recipe, std::string &message, const std::string &price)
{
    RapidJsonDocument result;
    const std::string account = g_cookServerTestConfig.m_userIdPrefix + "_review_" + name;
    if (!call(client, "", "/api/auth/register",
            "{\"account\":\"" + account + "\",\"password\":\"ReviewTest123!\",\"nickname\":\"Reviewer fixture\"}",
            result, message))
    {
        return false;
    }
    token = result.getStringOrDefault("token", "");
    if (!call(client, "", "/api/auth/login", "{\"account\":\"qmzdc\",\"password\":\"yangnan\"}", result, message))
    {
        return false;
    }
    admin = result.getStringOrDefault("token", "");
    if (!call(client, token, "/api/recipes/custom/publish", publication("nonce_" + name, price), result, message))
    {
        return false;
    }
    recipe = result.getValue("recipe");
    return check(!recipe.getStringOrDefault("id", "").empty(), "Missing server recipe ID", message);
}

std::string CookServerTestReviewCases::edit(const RapidJsonValue &recipe, int32_t amount)
{
    RapidJsonDocument body;
    body.addString("recipeId", recipe.getStringOrDefault("id", ""));
    body.addString("currentRevisionId", recipe.getStringOrDefault("currentRevisionId", ""));
    body.addInt("editorVersion", 2);
    body.addString("priceType", "coin");
    body.addInt("priceAmount", amount);
    body.addValue("steps", recipe.getValue("tasks"));
    return body.toString();
}

std::string CookServerTestReviewCases::decision(
    const RapidJsonValue &recipe, const std::string &selected, const std::string &reason)
{
    RapidJsonDocument body;
    body.addString(
        "reviewId", recipe.getStringOrDefault("reviewId", recipe.getStringOrDefault("currentRevisionId", "")));
    body.addString("decision", selected);
    body.addString("reason", reason);
    return body.toString();
}

void CookServerTestReviewCases::run(
    CookServerTestHttpClient &client, const CookServerTestFeedCases::RegisterFunction &registerCase)
{
    registerCase("免费自动发布与金币元宝共用待审队列",
        [&](std::string &message, int32_t &status) -> bool
        {
            std::string token, admin;
            RapidJsonValue recipe;
            RapidJsonDocument result;
            if (!fixture(client, "prices", token, admin, recipe, message))
            {
                return false;
            }
            if (!check(recipe.getStringOrDefault("reviewState", "") == "pending", "Paid recipe published before review",
                    message))
            {
                return false;
            }
            if (!call(client, token, "/api/recipes/custom/publish", publication("yuanbao", "yuanbao"), result, message))
            {
                return false;
            }
            if (!check(result.getValue("recipe").getStringOrDefault("reviewState", "") == "pending",
                    "Yuanbao bypassed review", message))
            {
                return false;
            }
            if (!call(client, token, "/api/recipes/custom/publish", publication("free", "free"), result, message))
            {
                return false;
            }
            if (!check(result.getValue("recipe").getStringOrDefault("status", "") == "published",
                    "Free recipe not published", message))
            {
                return false;
            }
            return call(client, admin, "/api/recipes/reviews/list?state=pending", "", result, message);
        });
    registerCase("审核通过才开放公开菜谱及购买",
        [&](std::string &message, int32_t &status) -> bool
        {
            std::string token, admin;
            RapidJsonValue recipe;
            RapidJsonDocument result;
            if (!fixture(client, "approve", token, admin, recipe, message))
            {
                return false;
            }
            const std::string id = recipe.getStringOrDefault("id", "");
            if (!call(client, admin, "/api/recipes/detail?recipeId=" + id, "", result, message, false))
            {
                return false;
            }
            if (!call(client, admin, "/api/recipes/reviews/decide", decision(recipe, "approved"), result, message))
            {
                return false;
            }
            if (!check(result.getValue("review").getStringOrDefault("state", "") == "approved",
                    "Decision not confirmed", message))
            {
                return false;
            }
            return call(client, admin, "/api/recipes/detail?recipeId=" + id, "", result, message);
        });
    registerCase("不通过原因回馈及作者拒绝审核权限",
        [&](std::string &message, int32_t &status) -> bool
        {
            std::string token, admin;
            RapidJsonValue recipe;
            RapidJsonDocument result;
            if (!fixture(client, "reject", token, admin, recipe, message))
            {
                return false;
            }
            if (!call(
                    client, token, "/api/recipes/reviews/decide", decision(recipe, "approved"), result, message, false))
            {
                return false;
            }
            if (!call(client, admin, "/api/recipes/reviews/decide", decision(recipe, "rejected", "Explain timing"),
                    result, message))
            {
                return false;
            }
            if (!call(client, token, "/api/recipes/creation/detail?recipeId=" + recipe.getStringOrDefault("id", ""), "",
                    result, message))
            {
                return false;
            }
            return check(result.getValue("recipe").getStringOrDefault("reviewReason", "") == "Explain timing",
                "Rejection reason lost", message);
        });
    registerCase("唯一创作ID及仅改价格的历史差异",
        [&](std::string &message, int32_t &status) -> bool
        {
            std::string token, admin;
            RapidJsonValue recipe;
            RapidJsonDocument result;
            if (!fixture(client, "history", token, admin, recipe, message))
            {
                return false;
            }
            const std::string creation = recipe.getStringOrDefault("creationId", "");
            if (!call(client, admin, "/api/recipes/reviews/decide", decision(recipe, "approved"), result, message))
            {
                return false;
            }
            if (!call(client, token, "/api/recipes/creation/save", edit(recipe, 8), result, message))
            {
                return false;
            }
            recipe = result.getValue("recipe");
            if (!check(!creation.empty() && recipe.getStringOrDefault("creationId", "") == creation &&
                           recipe.getIntOrDefault("reviewNumber", 0) == 2,
                    "Identity or review sequence changed", message))
            {
                return false;
            }
            if (!call(client, admin, "/api/recipes/reviews/detail?recipeId=" + recipe.getStringOrDefault("id", ""), "",
                    result, message))
            {
                return false;
            }
            return check(result.getValue("previousApprovedRecipe").getIntOrDefault("priceAmount", -1) == 7 &&
                             result.getArrayValueOrEmpty("history").size() == 2,
                "Previous approved price or history lost", message);
        });
    registerCase("普通账户每日五菜三次修改及重复提交不扣次",
        [&](std::string &message, int32_t &status) -> bool
        {
            std::string token, admin;
            RapidJsonValue recipe;
            RapidJsonDocument result;
            if (!fixture(client, "quota", token, admin, recipe, message))
            {
                return false;
            }
            for (int32_t i = 0; i < 4; ++i)
            {
                if (!call(client, token, "/api/recipes/custom/publish",
                        publication("quota_" + CStringManager::toStringInt32(i), "free"), result, message))
                {
                    return false;
                }
            }
            if (!call(
                    client, token, "/api/recipes/custom/publish", publication("sixth", "free"), result, message, false))
            {
                return false;
            }
            for (int32_t i = 0; i < 3; ++i)
            {
                if (!call(client, token, "/api/recipes/creation/save", edit(recipe, 8 + i), result, message))
                {
                    return false;
                }
                recipe = result.getValue("recipe");
            }
            if (!call(client, token, "/api/recipes/creation/save", edit(recipe, 11), result, message, false))
            {
                return false;
            }
            const std::string revision = recipe.getStringOrDefault("currentRevisionId", "");
            if (!call(client, token, "/api/recipes/creation/save", edit(recipe, 10), result, message))
            {
                return false;
            }
            return check(result.getValue("recipe").getStringOrDefault("currentRevisionId", "") == revision,
                "No-op created a new submission", message);
        });
    registerCase("相反审核同时提交仅一个持久结果",
        [&](std::string &message, int32_t &status) -> bool
        {
            std::string token, admin;
            RapidJsonValue recipe;
            if (!fixture(client, "race", token, admin, recipe, message))
            {
                return false;
            }
            std::atomic<int32_t> ready(0), successes(0), conflicts(0);
            std::vector<uint32_t> threads;
            std::vector<std::shared_ptr<CookServerTestStressTask>> tasks;
            for (int32_t i = 0; i < 2; ++i)
            {
                const uint32_t id = CTaskThreadManager::Instance().Init();
                const std::shared_ptr<CTaskThread> thread = CTaskThreadManager::Instance().GetThreadInterface(id);
                if (!thread)
                {
                    CTaskThreadManager::Instance().Uninit(id);
                    break;
                }
                std::shared_ptr<CookServerTestStressTask> task(new CookServerTestStressTask(
                    [&, i](const std::atomic<bool> &stop)
                    {
                        ++ready;
                        const std::chrono::high_resolution_clock::time_point start = CSystem::GetHighTickCount();
                        while (ready.load() < 2 && !stop.load() && CSystem::GetHighTickCountMilliRunTime(start) < 3000)
                        {
                            CSystem::Sleep(1);
                        }
                        CookServerTestHttpClient peer(g_cookServerTestConfig.m_host, g_cookServerTestConfig.m_port,
                            g_cookServerTestConfig.m_timeoutMs, g_cookServerTestConfig.m_maxResponseBytes);
                        std::map<std::string, std::string> headers;
                        headers["Authorization"] = "Bearer " + admin;
                        CookServerTestHttpResponse response;
                        peer.post("/api/recipes/reviews/decide", decision(recipe, i == 0 ? "approved" : "rejected"),
                            headers, response);
                        RapidJsonDocument json;
                        if (response.m_networkOk && response.m_statusCode == 200 && json.parse(response.m_body))
                        {
                            if (json.getBoolOrDefault("ok", false))
                            {
                                ++successes;
                            }
                            else if (json.getStringOrDefault("code", "") == "already_processed")
                            {
                                ++conflicts;
                            }
                        }
                    }));
                thread->PostTask(task, 1);
                tasks.push_back(task);
                threads.push_back(id);
            }
            for (size_t i = 0; i < threads.size(); ++i)
            {
                CTaskThreadManager::Instance().WaitForEnd(threads[i]);
            }
            return check(threads.size() == 2 && successes == 1 && conflicts == 1,
                "Concurrent review was not exactly once", message);
        });
    registerCase("审核版本媒体字段保留并禁止非法文件引用",
        [&](std::string &message, int32_t &status) -> bool
        {
            std::string token, admin;
            RapidJsonValue recipe;
            RapidJsonDocument result, body;
            if (!fixture(client, "media", token, admin, recipe, message))
            {
                return false;
            }
            body.parse(edit(recipe, 8));
            body.addStringArray("mediaIds", std::vector<std::string>(1, "nonexistent_file"));
            if (!call(client, token, "/api/recipes/creation/save", body.toString(), result, message, false))
            {
                return false;
            }
            if (!call(client, token, "/api/recipes/reviews/detail?recipeId=" + recipe.getStringOrDefault("id", ""), "",
                    result, message))
            {
                return false;
            }
            return check(result.getArrayValueOrEmpty("history").size() == 1 &&
                             result.getValue("recipe").getIntOrDefault("priceAmount", -1) == 7,
                "Rejected media reference consumed history", message);
        });
    registerCase("新提交替代旧待审且已审核按实际决定记录",
        [&](std::string &message, int32_t &status) -> bool
        {
            std::string token, admin;
            RapidJsonValue recipe;
            RapidJsonDocument result;
            if (!fixture(client, "replace", token, admin, recipe, message))
            {
                return false;
            }
            const RapidJsonValue previous = recipe;
            if (!call(client, token, "/api/recipes/creation/save", edit(recipe, 8), result, message))
            {
                return false;
            }
            recipe = result.getValue("recipe");
            if (!call(client, admin, "/api/recipes/reviews/decide", decision(previous, "approved"), result, message,
                    false))
            {
                return false;
            }
            if (!call(client, admin, "/api/recipes/reviews/decide", decision(recipe, "rejected"), result, message))
            {
                return false;
            }
            if (!check(result.getValue("review").getIntOrDefault("reviewedAtSeconds", 0) > 0, "Review time missing",
                    message))
            {
                return false;
            }
            return call(client, admin, "/api/recipes/reviews/list?state=processed", "", result, message);
        });
    registerCase("资源迁移不覆盖冲突且重试逐文件校验",
        [&](std::string &message, int32_t &status) -> bool
        {
            const std::string root = CSystem::joinPath(CSystem::GetCurrentExePath(), "migration_" + CSystem::uuid(0));
            const std::string source = CSystem::joinPath(root, "legacy"), target = CSystem::joinPath(root, "external");
            if (!CSystem::createDirectories(source) || !CSystem::createDirectories(target))
            {
                return check(false, "Cannot prepare migration fixture", message);
            }
            {
                std::ofstream out(CSystem::joinPath(source, "media.bin").c_str(), std::ios::binary);
                out << "original resource";
            }
            {
                std::ofstream out(CSystem::joinPath(target, "media.bin").c_str(), std::ios::binary);
                out << "conflicting resource";
            }
            std::string error;
            if (!check(!CSystem::migrateDirectory(source, target, error) &&
                           CSystem::DirOrFileExist(CSystem::joinPath(source, "media.bin")),
                    "Migration overwrote conflict", message))
            {
                return false;
            }
            {
                std::ofstream out(CSystem::joinPath(target, "media.bin").c_str(), std::ios::binary | std::ios::trunc);
                out << "original resource";
            }
            if (!CSystem::migrateDirectory(source, target, error))
            {
                return check(false, error, message);
            }
            return check(!CSystem::DirOrFileExist(source) && CSystem::migrateDirectory(source, target, error),
                "Migration is not idempotent", message);
        });
}