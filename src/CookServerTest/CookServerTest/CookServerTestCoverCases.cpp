#include "CookServerTestCoverCases.h"
#include "CookServerTestConfig.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "RapidJson/RapidJsonAPI.h"
#include <stdexcept>

void CookServerTestCoverCases::Run(CookServerTestHttpClient& client, const CookServerTestFeedCases::RegisterFunction& registerCase)
{
    registerCase("真实正方形封面上传绑定及跨账号隔离", [&](std::string& message, int32_t& statusCode) -> bool
    {
        try
        {
            std::string token;
            std::map<std::string, std::string> headers;
            const auto request = [&](const std::string& path, const std::string& body, bool expected) -> RapidJsonDocument
            {
                CookServerTestHttpResponse response;
                client.request(body.empty() ? "GET" : "POST", path, body, headers, response);
                statusCode = response.m_statusCode;
                RapidJsonDocument json;
                if (!response.m_networkOk || statusCode != 200 || !json.parse(response.m_body) || json.getBoolOrDefault("ok", !expected) != expected)
                    throw std::runtime_error(path + " " + response.m_body);
                return json;
            };
            const std::string name = g_cookServerTestConfig.m_userIdPrefix + "_cover";
            token = request("/api/auth/register", "{\"account\":\"" + name + "\",\"password\":\"cookTest123\",\"nickname\":\"Cover author\"}", true).getStringOrDefault("token", "");
            headers["Authorization"] = "Bearer " + token;
            const auto upload = [&](const std::string& type, const std::string& extension, const std::string& bytes, bool expected) -> std::string
            {
                const RapidJsonDocument started = request("/api/videos/upload/start", "{\"mediaType\":\"" + type + "\",\"fileName\":\"test" + extension + "\",\"fileSizeBytes\":" + CStringManager::toStringInt32(static_cast<int32_t>(bytes.size())) + "}", true);
                const std::string id = started.getStringOrDefault("uploadId", "");
                for (size_t offset = 0; offset < bytes.size(); offset += 512 * 1024)
                    request("/api/videos/upload/chunk?uploadId=" + id + "&offset=" + CStringManager::toStringInt64(offset), bytes.substr(offset, 512 * 1024), true);
                const RapidJsonDocument completed = request("/api/videos/upload/complete", "{\"uploadId\":\"" + id + "\"}", expected);
                return completed.getValue("video").getStringOrDefault("id", "");
            };
            const unsigned char squareData[] = {137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,1,0,0,0,1,8,2,0,0,0,144,119,83,222,0,0,0,12,73,68,65,84,120,156,99,104,104,104,0,0,3,4,1,129,75,211,210,16,0,0,0,0,73,69,78,68,174,66,96,130};
            const unsigned char rectangleData[] = {137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,2,0,0,0,1,8,2,0,0,0,123,64,232,221,0,0,0,11,73,68,65,84,120,156,99,104,0,3,0,10,135,3,1,196,198,31,102,0,0,0,0,73,69,78,68,174,66,96,130};
            const std::string square(reinterpret_cast<const char*>(squareData), sizeof(squareData));
            const std::string rectangle(reinterpret_cast<const char*>(rectangleData), sizeof(rectangleData));
            const std::string imageId = upload("image", ".png", square, true);
            upload("image", ".png", rectangle, false);
            upload("image", ".jpg", "not an image", false);
            CookServerTestHttpResponse hidden;
            client.get("/api/videos/file?videoId=" + imageId, headers, hidden);
            if (hidden.m_statusCode != 404) throw std::runtime_error("Unpublished image readable");
            const std::string recipeId = name + "_recipe";
            const std::string recipe = "{\"recipe\":{\"schemaVersion\":\"1.0\",\"id\":\"" + recipeId + "\",\"title\":\"Cover recipe\",\"category\":\"\\u852c\\u83dc\",\"priceType\":\"free\",\"coverImageId\":\"" + imageId + "\",\"tasks\":[{\"id\":\"cut\",\"title\":\"Cut\",\"durationSeconds\":10,\"active\":true}]}}";
            request("/api/recipes/custom/publish", recipe, true);
            CookServerTestHttpResponse image;
            client.get("/api/videos/file?videoId=" + imageId, headers, image);
            if (!image.m_networkOk || image.m_statusCode != 200 || image.m_body != square) throw std::runtime_error("Published cover bytes differ");
            request("/api/videos/detail?videoId=" + imageId, "", false);
            const std::string fixture = CSystem::readFile(CSystem::GetCurrentExePath() + "../CookServer/res/video/cook_000001.mp4");
            if (fixture.empty()) throw std::runtime_error("Missing upload video fixture");
            const std::string videoId = upload("video", ".mp4", fixture, true);
            request("/api/videos/publish", "{\"videoId\":\"" + videoId + "\",\"recipeIds\":[\"" + recipeId + "\"],\"coverImageId\":\"" + imageId + "\"}", true);
            const RapidJsonValue detail = request("/api/recipes/detail?recipeId=" + recipeId, "", true).getValue("recipe");
            if (detail.getStringOrDefault("coverImageId", "") != imageId || detail.getStringOrDefault("videoId", "") != videoId)
                throw std::runtime_error("Cover/original video references do not match publication");
            const RapidJsonDocument stranger = request("/api/auth/register", "{\"account\":\"" + name + "_peer\",\"password\":\"cookTest123\",\"nickname\":\"Peer\"}", true);
            headers["Authorization"] = "Bearer " + stranger.getStringOrDefault("token", "");
            std::string foreign = recipe;
            foreign.replace(foreign.find(recipeId), recipeId.size(), recipeId + "_peer");
            request("/api/recipes/custom/publish", foreign, false);
            return true;
        }
        catch (const std::exception& error) { message = error.what(); return false; }
    });
}