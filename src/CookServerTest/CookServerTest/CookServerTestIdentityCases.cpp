#include "CookServerTestIdentityCases.h"
#include "CookServerTestConfig.h"
#include "CookServerTestHelper.h"
#include <stdexcept>
#include <map>

void CookServerTestIdentityCases::require(bool value, const char* message)
{
    if (!value)
    {
        throw std::runtime_error(message);
    }
}

RapidJsonDocument CookServerTestIdentityCases::request(CookServerTestHttpClient& client, const std::string& token,
    const std::string& path, const std::string& body, bool ok)
{
    std::map<std::string, std::string> headers;
    headers["Authorization"] = "Bearer " + token;
    CookServerTestHttpResponse response;
    client.request(body.empty() ? "GET" : "POST", path, body, headers, response);
    RapidJsonDocument json;
    require(response.m_networkOk && response.m_statusCode == 200, "HTTP failed");
    require(json.parse(response.m_body) && json.getBoolOrDefault("ok", !ok) == ok, "Unexpected JSON result");
    return json;
}

RapidJsonDocument CookServerTestIdentityCases::user(CookServerTestHttpClient& client, const std::string& suffix)
{
    return request(client, "", "/api/auth/register", "{\"account\":\"" + g_cookServerTestConfig.m_userIdPrefix +
        "_avatar_" + suffix + "\",\"password\":\"AvatarTest123!\",\"nickname\":\"Avatar\",\"role\":\"super_admin\"}");
}

RapidJsonDocument CookServerTestIdentityCases::upload(CookServerTestHttpClient& client, const std::string& token,
    const std::string& bytes, bool ok, const std::string& query)
{
    std::map<std::string, std::string> headers;
    headers["Authorization"] = "Bearer " + token;
    headers["Content-Type"] = "application/octet-stream";
    CookServerTestHttpResponse response;
    client.request("POST", "/api/users/avatar/upload" + query, bytes, headers, response);
    RapidJsonDocument json;
    require(response.m_networkOk && response.m_statusCode == 200, "Avatar HTTP failed");
    require(json.parse(response.m_body) && json.getBoolOrDefault("ok", !ok) == ok, "Avatar validation result differs");
    return json;
}

std::string CookServerTestIdentityCases::png(bool square)
{
    const unsigned char squareData[] = {137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,1,0,0,0,1,8,2,0,0,0,144,119,83,222,0,0,0,12,73,68,65,84,120,156,99,104,104,104,0,0,3,4,1,129,75,211,210,16,0,0,0,0,73,69,78,68,174,66,96,130};
    const unsigned char rectangleData[] = {137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,2,0,0,0,1,8,2,0,0,0,123,64,232,221,0,0,0,11,73,68,65,84,120,156,99,104,0,3,0,10,135,3,1,196,198,31,102,0,0,0,0,73,69,78,68,174,66,96,130};
    return square ? std::string(reinterpret_cast<const char*>(squareData), sizeof(squareData)) :
        std::string(reinterpret_cast<const char*>(rectangleData), sizeof(rectangleData));
}

std::string CookServerTestIdentityCases::jpeg()
{
    // JPEG由独立图像工具生成；不是扩展名伪装的PNG。
    const unsigned char data[] = { 255,216,255,224,0,16,74,70,73,70,0,1,1,1,0,96,0,96,0,0,255,219,0,67,0,8,6,6,7,6,5,8,7,7,7,9,9,8,10,12,20,13,12,11,11,12,25,18,19,15,20,29,26,31,30,29,26,28,28,32,36,46,39,32,34,44,35,28,28,40,55,41,44,48,49,52,52,52,31,39,57,61,56,50,60,46,51,52,50,255,219,0,67,1,9,9,9,12,11,12,24,13,13,24,50,33,28,33,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,255,192,0,17,8,0,2,0,2,3,1,34,0,2,17,1,3,17,1,255,196,0,31,0,0,1,5,1,1,1,1,1,1,0,0,0,0,0,0,0,0,1,2,3,4,5,6,7,8,9,10,11,255,196,0,181,16,0,2,1,3,3,2,4,3,5,5,4,4,0,0,1,125,1,2,3,0,4,17,5,18,33,49,65,6,19,81,97,7,34,113,20,50,129,145,161,8,35,66,177,193,21,82,209,240,36,51,98,114,130,9,10,22,23,24,25,26,37,38,39,40,41,42,52,53,54,55,56,57,58,67,68,69,70,71,72,73,74,83,84,85,86,87,88,89,90,99,100,101,102,103,104,105,106,115,116,117,118,119,120,121,122,131,132,133,134,135,136,137,138,146,147,148,149,150,151,152,153,154,162,163,164,165,166,167,168,169,170,178,179,180,181,182,183,184,185,186,194,195,196,197,198,199,200,201,202,210,211,212,213,214,215,216,217,218,225,226,227,228,229,230,231,232,233,234,241,242,243,244,245,246,247,248,249,250,255,196,0,31,1,0,3,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,1,2,3,4,5,6,7,8,9,10,11,255,196,0,181,17,0,2,1,2,4,4,3,4,7,5,4,4,0,1,2,119,0,1,2,3,17,4,5,33,49,6,18,65,81,7,97,113,19,34,50,129,8,20,66,145,161,177,193,9,35,51,82,240,21,98,114,209,10,22,36,52,225,37,241,23,24,25,26,38,39,40,41,42,53,54,55,56,57,58,67,68,69,70,71,72,73,74,83,84,85,86,87,88,89,90,99,100,101,102,103,104,105,106,115,116,117,118,119,120,121,122,130,131,132,133,134,135,136,137,138,146,147,148,149,150,151,152,153,154,162,163,164,165,166,167,168,169,170,178,179,180,181,182,183,184,185,186,194,195,196,197,198,199,200,201,202,210,211,212,213,214,215,216,217,218,226,227,228,229,230,231,232,233,234,242,243,244,245,246,247,248,249,250,255,218,0,12,3,1,0,2,17,3,17,0,63,0,245,15,14,120,71,195,87,62,23,210,103,159,195,218,76,179,75,101,11,201,36,150,81,179,59,20,4,146,74,228,146,123,209,69,21,246,120,127,224,195,209,126,70,21,62,54,127,255,217 };
    return std::string(reinterpret_cast<const char*>(data), sizeof(data));
}

void CookServerTestIdentityCases::run(CookServerTestHttpClient& client, const CookServerTestFeedCases::RegisterFunction& registerCase)
{
    registerCase("头像PNG/JPEG上传、版本替换与公开资料一致", [&](std::string& message, int32_t& status) -> bool
    {
        try
        {
            const RapidJsonDocument a = user(client, "a"), b = user(client, "b");
            const std::string token = a.getStringOrDefault("token", "");
            const std::string id = a.getValue("account").getStringOrDefault("userId", "");
            const std::string peer = b.getValue("account").getStringOrDefault("userId", "");
            const std::string first = upload(client, token, png()).getValue("profile").getStringOrDefault("avatarUrl", "");
            CookServerTestHttpResponse image;
            client.get(first, image);
            require(image.m_networkOk && image.m_statusCode == 200 && image.m_body == png(), "Stored PNG bytes differ");
            const std::string second = upload(client, token, jpeg()).getValue("profile").getStringOrDefault("avatarUrl", "");
            require(!second.empty() && second != first, "Avatar version did not change");
            client.get(second, image);
            require(image.m_statusCode == 200 && image.m_body == jpeg(), "Stored JPEG bytes differ");
            client.get(first, image);
            require(image.m_statusCode == 404, "Replaced image is still exposed");
            const std::string bt = b.getStringOrDefault("token", "");
            require(request(client, bt, "/api/users/profile?userId=" + id).getValue("profile").getStringOrDefault("avatarUrl", "") == second, "Public avatar mismatch");
            require(request(client, token, "/api/users/profile?userId=" + peer).getValue("profile").getStringOrDefault("avatarUrl", "").empty(), "Another user's avatar changed");
            status = 200;
            return true;
        }
        catch (const std::exception& error)
        {
            message = error.what();
            return false;
        }
    });
    registerCase("头像伪造身份、损坏图片与大小边界不覆盖原图", [&](std::string& message, int32_t& status) -> bool
    {
        try
        {
            const RapidJsonDocument a = user(client, "limits");
            const std::string token = a.getStringOrDefault("token", "");
            const std::string id = a.getValue("account").getStringOrDefault("userId", "");
            const std::string url = upload(client, token, png()).getValue("profile").getStringOrDefault("avatarUrl", "");
            upload(client, token, "", false);
            upload(client, token, "invalid", false);
            upload(client, token, png(false), false);
            upload(client, token, oversizedPng(), false);
            upload(client, token, png().substr(0, 20), false);
            upload(client, token, jpeg().substr(0, 20), false);
            upload(client, token, std::string(512 * 1024 + 1, 'x'), false);
            upload(client, token, png(), false, "?userId=qmzdc");
            std::map<std::string, std::string> invalid;
            invalid["Authorization"] = "Bearer invalid";
            CookServerTestHttpResponse response;
            client.request("POST", "/api/users/avatar/upload", png(), invalid, response);
            require(response.m_networkOk && response.m_statusCode == 401, "Anonymous avatar write allowed");
            client.get("/api/users/avatar?userId=" + id + "&key=../../CookServer.ini", response);
            require(response.m_statusCode == 404, "Invalid avatar path accepted");
            require(request(client, token, "/api/users/profile?userId=" + id).getValue("profile").getStringOrDefault("avatarUrl", "") == url, "Failed upload replaced original avatar");
            std::string padded = png();
            padded.resize(512 * 1024, '\0');
            upload(client, token, padded);
            status = 200;
            return true;
        }
        catch (const std::exception& error)
        {
            message = error.what();
            return false;
        }
    });
    registerCase("qmzdc登录、角色分类与系统作品归属", [&](std::string& message, int32_t& status) -> bool
    {
        try
        {
            const RapidJsonDocument login = request(client, "", "/api/auth/login", "{\"account\":\"qmzdc\",\"password\":\"yangnan\"}");
            const RapidJsonValue account = login.getValue("account");
            require(account.getStringOrDefault("userId", "") == "qmzdc" && account.getStringOrDefault("role", "") == "super_admin", "System identity or role differs");
            require(account.getStringOrDefault("nickname", "") == CookServerTestHelper::localTextToUtf8("全民做大厨"), "System nickname differs");
            const std::string token = login.getStringOrDefault("token", "");
            request(client, "", "/api/auth/login", "{\"account\":\"qmzdc\",\"password\":\"wrong-pass\"}", false);
            request(client, "", "/api/auth/register", "{\"account\":\"qmzdc\",\"password\":\"AvatarTest123!\"}", false);
            const std::vector<RapidJsonValue> defaults = request(client, token, "/api/recipes").getArrayValueOrEmpty("recipes");
            int32_t count = 0;
            for (size_t i = 0; i < defaults.size(); ++i)
            {
                const std::string recipe = defaults[i].getStringOrDefault("id", "");
                if (recipe.compare(0, 10, "cook_00000") == 0)
                {
                    require(defaults[i].getStringOrDefault("authorUserId", "") == "qmzdc" && defaults[i].getBoolOrDefault("owned", false), "Built-in recipe has another owner");
                    ++count;
                }
            }
            require(count == 7, "Missing built-in recipe ownership");
            const RapidJsonDocument ordinary = user(client, "role");
            require(ordinary.getValue("account").getStringOrDefault("role", "") == "user", "Client forged elevated role");
            const std::string ot = ordinary.getStringOrDefault("token", "");
            const std::string prefix = g_cookServerTestConfig.m_userIdPrefix + "_system_";
            for (int32_t paid = 0; paid < 2; ++paid)
            {
                const std::string recipeId = prefix + (paid != 0 ? "paid" : "free");
                const std::string body = "{\"recipe\":{\"schemaVersion\":\"1.0\",\"id\":\"" + recipeId + "\",\"title\":\"System test\",\"category\":\"\\u852c\\u83dc\",\"priceType\":\"" + (paid != 0 ? "coin" : "free") + "\",\"priceAmount\":" + (paid != 0 ? "1" : "0") + ",\"tasks\":[{\"id\":\"cut\",\"title\":\"Cut\",\"durationSeconds\":10,\"active\":true}]}}";
                const RapidJsonValue submitted = request(client, token, "/api/recipes/custom/publish", body).getValue("recipe");
                if (paid != 0) request(client, token, "/api/recipes/reviews/decide", "{\"reviewId\":\"" + submitted.getStringOrDefault("reviewId", "") + "\",\"decision\":\"approved\"}");
                const RapidJsonValue recipe = request(client, ot, "/api/recipes/detail?recipeId=" + submitted.getStringOrDefault("id", "")).getValue("recipe");
                require(recipe.getStringOrDefault("authorUserId", "") == "qmzdc" && recipe.getBoolOrDefault("systemRecipe", false), "Published system author differs");
                require(recipe.getBoolOrDefault("inMenu", false) == (paid == 0), "System paid/free default menu differs");
                require(recipe.getBoolOrDefault("owned", false) == (paid == 0), "System paid/free access differs");
            }
            status = 200;
            return true;
        }
        catch (const std::exception& error)
        {
            message = error.what();
            return false;
        }
    });
}

std::string CookServerTestIdentityCases::oversizedPng()
{
    const unsigned char data[] = {
        137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,4,1,0,0,4,1,
        8,6,0,0,0,91,131,147,24,0,0,0,1,115,82,71,66,0,174,206,28,233,0,0,
        0,4,103,65,77,65,0,0,177,143,11,252,97,5,0,0,0,9,112,72,89,115,0,0,
        14,195,0,0,14,195,1,199,111,168,100,0,0,16,0,73,68,65,84,120,94,237,193,1,
        13,0,0,0,194,160,247,79,109,14,55,32,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,128,95,53,39,197,0,1,15,98,143,146,0,0,0,0,73,69,78,68,174,
        66,96,130,
    };
    return std::string(reinterpret_cast<const char*>(data), sizeof(data));
}