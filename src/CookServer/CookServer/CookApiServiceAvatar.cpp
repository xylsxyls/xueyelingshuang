#include "CookApiService.h"
#include "CookAvatarHelper.h"
#include "CookHelper.h"
#include "CookServerHelper.h"
#include "RapidJson/RapidJsonAPI.h"

bool CookApiService::initialize(std::string& message)
{
    return m_accountStore.initialize(message);
}

HttpResponse CookApiService::buildAvatarResponse(const HttpRequest& request)
{
    const std::string userId = CookHelper::queryValue(request, "userId");
    const std::string requestedKey = CookHelper::queryValue(request, "key");
    std::string currentKey;
    HttpResponse response;
    if (userId.empty() || !CookAvatarHelper::isKey(requestedKey) ||
        !m_accountStore.getAvatarKey(userId, currentKey) || currentKey != requestedKey ||
        !CookAvatarHelper::read(currentKey, response.m_body, response.m_contentType))
    {
        return errorResponse("Avatar is unavailable", 404, "AVATAR_NOT_FOUND");
    }
    response.m_statusCode = 200;
    response.m_headers["Cache-Control"] = "public, max-age=3600, immutable";
    response.m_headers["X-Content-Type-Options"] = "nosniff";
    return response;
}

HttpResponse CookApiService::uploadAvatarResponse(const HttpRequest& request, const std::string& userId)
{
    const std::string target = CookHelper::queryValue(request, "userId");
    std::string key, oldKey, message;
    bool ok = target.empty() || target == userId;
    if (!ok)
    {
        message = "Only your own avatar can be updated";
    }
    if (ok)
    {
        ok = CookAvatarHelper::save(request.m_body, key, message);
    }
    if (ok)
    {
        ok = m_accountStore.updateAvatar(userId, key, oldKey, message);
        if (!ok)
        {
            CookAvatarHelper::remove(key);
        }
    }
    RapidJsonDocument result;
    result.setObject();
    result.addBool("ok", ok);
    CookServerHelper::addString(result, "message", message);
    if (ok)
    {
        UserContactInfo profile;
        if (m_accountStore.getPublicProfile(userId, userId, profile))
        {
            result.addValue("profile", publicProfileToJson(profile));
        }
        CookAvatarHelper::remove(oldKey);
    }
    return CookServerHelper::jsonResponse(result.toString());
}