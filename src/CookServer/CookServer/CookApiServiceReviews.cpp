#include "CookApiService.h"
#include "CookHelper.h"
#include "CookServerHelper.h"
#include "CStringManager/CStringManagerAPI.h"
#include "RapidJson/RapidJsonAPI.h"
#include <limits.h>

std::string CookApiService::buildReviewJson(const HttpRequest &request, const std::string &userId)
{
    RapidJsonDocument result;
    std::string message;
    bool ok = false;
    const std::string countText = CookHelper::queryValue(request, "count");
    const int64_t countValue = countText.empty() ? 18 : CStringManager::atoi64(countText.c_str());
    if (countValue < 1 || countValue > INT32_MAX || countText.size() > 10 ||
        countText.find_first_not_of("0123456789") != std::string::npos)
    {
        result.addBool("ok", false);
        result.addString("message", "Invalid review page size");
        return result.toString();
    }
    if (request.m_method == "GET" && request.m_uri == "/api/recipes/reviews/list")
    {
        ok = m_accountStore.listRecipeReviews(userId, CookHelper::queryValue(request, "state"),
            CookHelper::queryValue(request, "cursor"), static_cast<int32_t>(countValue), result, message);
    }
    else if (request.m_method == "GET" && request.m_uri == "/api/recipes/reviews/history")
    {
        ok = m_accountStore.listRecipeReviewHistory(userId, CookHelper::queryValue(request, "recipeId"),
            CookHelper::queryValue(request, "cursor"), static_cast<int32_t>(countValue), result, message);
    }
    else if (request.m_method == "GET")
    {
        ok = m_accountStore.getRecipeReview(userId, CookHelper::queryValue(request, "recipeId"),
            CookHelper::queryValue(request, "reviewId"), result, message);
    }
    else
    {
        RapidJsonDocument body;
        const std::string missing(1, '\x01');
        if (parseBody(request.m_body, body) && body.getStringOrDefault("reviewId", missing) != missing &&
            body.getStringOrDefault("decision", missing) != missing)
        {
            const std::string reason = body.getStringOrDefault("reason", "");
            RapidJson members;
            const bool hasReason = members.parse(request.m_body) && members.hasMember("reason");
            if (!hasReason || body.getStringOrDefault("reason", missing) != missing)
            {
                ok = m_accountStore.decideRecipeReview(userId, body.getStringOrDefault("reviewId", ""),
                    body.getStringOrDefault("decision", ""), reason, result, message);
            }
            else
            {
                message = "审核说明必须是文本";
            }
        }
        else
        {
            message = "审核请求格式无效";
        }
    }
    result.addBool("ok", ok);
    CookServerHelper::addString(result, "message", message);
    return result.toString();
}

std::string CookApiService::mediaViewer(const HttpRequest &request)
{
    if (headerValue(request, "Authorization").empty())
    {
        return "";
    }
    std::string userId;
    UserAccount account;
    HttpResponse failed;
    return authorize(request, userId, account, failed) ? userId : "";
}