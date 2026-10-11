#include "CookServerTestMediaCases.h"
#include "CookServerTestConfig.h"
#include "RapidJson/RapidJsonAPI.h"

bool CookServerTestMediaCases::check(const CookServerTestHttpResponse& response, int32_t status,
                                     bool ok, std::string& message, int32_t& statusCode)
{
    statusCode = response.m_statusCode;
    RapidJsonDocument document;
    if (!response.m_networkOk || statusCode != status || !document.parse(response.m_body) ||
        !document.isObject() || document.getBoolOrDefault("ok", !ok) != ok)
    {
        message = "unexpected media response status/body or network failure: " + response.m_error + " " + response.m_body;
        return false;
    }
    return true;
}

std::string CookServerTestMediaCases::start(CookServerTestHttpClient& client, std::string& message, int32_t& statusCode)
{
    CookServerTestHttpResponse response;
    client.post("/api/videos/upload/start", "{\"mediaType\":\"video\",\"fileName\":\"edge.mp4\",\"fileSizeBytes\":4}", response);
    if (!check(response, 200, true, message, statusCode)) return std::string();
    RapidJsonDocument document;
    document.parse(response.m_body);
    const std::string id = document.getStringOrDefault("uploadId", "");
    if (id.empty()) message = "missing upload session ID";
    return id;
}

void CookServerTestMediaCases::cancel(CookServerTestHttpClient& client, const std::string& id)
{
    if (id.empty()) return;
    CookServerTestHttpResponse response;
    client.post("/api/videos/upload/cancel", "{\"uploadId\":\"" + id + "\"}", response);
}

void CookServerTestMediaCases::run(CookServerTestHttpClient& client, const std::string& token,
                                   const std::string& videoId, const RegisterFunction& registerCase)
{
    client.setDefaultToken(token);
    registerCase("上传参数边界", [&client](std::string& message, int32_t& statusCode) -> bool
    {
        const char* bodies[] = {"{}", "{\"mediaType\":\"video\",\"fileName\":\"x.mp4\",\"fileSizeBytes\":-1}",
            "{\"mediaType\":\"video\",\"fileName\":\"x.mp4\",\"fileSizeBytes\":2147483647}",
            "{\"mediaType\":\"video\",\"fileName\":\"x.exe\",\"fileSizeBytes\":4}"};
        for (size_t i = 0; i < sizeof(bodies) / sizeof(bodies[0]); ++i)
        {
            CookServerTestHttpResponse response;
            client.post("/api/videos/upload/start", bodies[i], response);
            if (!check(response, 200, false, message, statusCode)) return false;
        }
        return true;
    });
    registerCase("重复分块与偏移边界", [&client](std::string& message, int32_t& statusCode) -> bool
    {
        const std::string id = start(client, message, statusCode);
        if (id.empty()) return false;
        std::map<std::string, std::string> headers;
        headers["Content-Type"] = "application/octet-stream";
        const std::string path = "/api/videos/upload/chunk?uploadId=" + id + "&offset=";
        const char* offsets[] = {"-1", "1", "4", "bad"};
        bool valid = true;
        for (size_t i = 0; i < sizeof(offsets) / sizeof(offsets[0]) && valid; ++i)
        {
            CookServerTestHttpResponse response;
            client.post(path + offsets[i], "x", headers, response);
            valid = check(response, 200, false, message, statusCode);
        }
        const char* chunks[] = {"", "ab", "ab", "xy", "cdef"};
        const bool expected[] = {false, true, true, false, false};
        for (size_t i = 0; i < sizeof(chunks) / sizeof(chunks[0]) && valid; ++i)
        {
            CookServerTestHttpResponse response;
            client.post(path + "0", chunks[i], headers, response);
            valid = check(response, 200, expected[i], message, statusCode);
            if (valid && expected[i] && response.m_body.find("\"receivedBytes\":2") == std::string::npos)
            { message = "duplicate chunk advanced received bytes"; valid = false; }
        }
        cancel(client, id);
        return valid;
    });
    registerCase("上传会话所有权与取消", [&client](std::string& message, int32_t& statusCode) -> bool
    {
        const std::string id = start(client, message, statusCode);
        if (id.empty()) return false;
        CookServerTestHttpResponse registration;
        client.post("/api/auth/register", "{\"account\":\"" + g_cookServerTestConfig.m_userIdPrefix +
            "_mediaother\",\"password\":\"cook1234\",\"nickname\":\"media peer\"}", registration);
        RapidJsonDocument peer;
        peer.parse(registration.m_body);
        const std::string peerToken = peer.getStringOrDefault("token", "");
        if (!check(registration, 200, true, message, statusCode) || peerToken.empty())
        { cancel(client, id); return false; }
        std::map<std::string, std::string> headers;
        const char* paths[] = {"/api/videos/upload/chunk?offset=0&uploadId=", "/api/videos/upload/cancel", "/api/videos/upload/complete"};
        bool valid = true;
        for (size_t identity = 0; identity < 2 && valid; ++identity)
        {
            headers["Authorization"] = "Bearer " + (identity == 0 ? peerToken : "invalid_edge_token");
            for (size_t i = 0; i < 3 && valid; ++i)
            {
                CookServerTestHttpResponse response;
                client.post(i == 0 ? std::string(paths[i]) + id : paths[i], i == 0 ? "ab" : "{\"uploadId\":\"" + id + "\"}", headers, response);
                valid = check(response, identity == 0 ? 200 : 401, false, message, statusCode);
            }
        }
        CookServerTestHttpResponse canceled;
        client.post("/api/videos/upload/cancel", "{\"uploadId\":\"" + id + "\"}", canceled);
        valid = check(canceled, 200, true, message, statusCode) && valid;
        CookServerTestHttpResponse finished;
        client.post("/api/videos/upload/complete", "{\"uploadId\":\"" + id + "\"}", finished);
        return check(finished, 200, false, message, statusCode) && valid;
    });
    registerCase("未完成上传不可完成", [&client](std::string& message, int32_t& statusCode) -> bool
    {
        const std::string id = start(client, message, statusCode);
        if (id.empty()) return false;
        CookServerTestHttpResponse response;
        client.post("/api/videos/upload/complete", "{\"uploadId\":\"" + id + "\"}", response);
        const bool valid = check(response, 200, false, message, statusCode);
        cancel(client, id);
        return valid;
    });
    registerCase("单账号并发上传上限", [&client](std::string& message, int32_t& statusCode) -> bool
    {
        const std::string first = start(client, message, statusCode);
        const std::string second = start(client, message, statusCode);
        CookServerTestHttpResponse response;
        client.post("/api/videos/upload/start", "{\"mediaType\":\"video\",\"fileName\":\"edge.mp4\",\"fileSizeBytes\":4}", response);
        bool valid = !first.empty() && !second.empty() && first != second && check(response, 200, false, message, statusCode);
        cancel(client, first);
        const std::string replacement = start(client, message, statusCode);
        valid = valid && !replacement.empty();
        cancel(client, second);
        cancel(client, replacement);
        return valid;
    });
    registerCase("分段参数边界", [&client, videoId](std::string& message, int32_t& statusCode) -> bool
    {
        if (videoId.empty()) { message = "missing real video fixture ID"; return false; }
        const char* ranges[] = {"", "&startSeconds=-1&durationSeconds=3", "&startSeconds=0&durationSeconds=0",
            "&startSeconds=0&durationSeconds=2147483647", "&startSeconds=1x&durationSeconds=3"};
        for (size_t i = 0; i < sizeof(ranges) / sizeof(ranges[0]); ++i)
        {
            CookServerTestHttpResponse response;
            client.get("/api/videos/segment?videoId=" + videoId + ranges[i], response);
            if (!check(response, 400, false, message, statusCode)) return false;
        }
        return true;
    });
    registerCase("文件Range边界", [&client, videoId](std::string& message, int32_t& statusCode) -> bool
    {
        std::map<std::string, std::string> headers;
        headers["Range"] = "bytes=2147483647-";
        CookServerTestHttpResponse invalid;
        client.get("/api/videos/file?videoId=" + videoId, headers, invalid);
        statusCode = invalid.m_statusCode;
        if (!invalid.m_networkOk || statusCode != 416 || invalid.m_headers["Content-Range"].find("bytes */") != 0)
        { message = "out-of-file range must return 416 and total length"; return false; }
        headers["Range"] = "bytes=-4";
        CookServerTestHttpResponse suffix;
        client.get("/api/videos/file?videoId=" + videoId, headers, suffix);
        statusCode = suffix.m_statusCode;
        if (!suffix.m_networkOk || statusCode != 206 || suffix.m_body.size() != 4)
        { message = "suffix range must return exactly four bytes"; return false; }
        return true;
    });
}