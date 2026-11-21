#include "HttpRequestWork.h"
#include "CEncodeDecode/CEncodeDecodeAPI.h"
#include <algorithm>
#include <cctype>
#include <map>
#include <mutex>
#ifndef __EMSCRIPTEN__
#include <chrono>
#endif

static std::mutex g_requestMutex;
static std::map<uint32_t, std::shared_ptr<HttpRequestWork>> g_requests;
static uint32_t g_nextRequest = 0;
// 以下配置在首个请求成功提交后冻结，读写均持有请求登记表锁。
static bool g_started = false;
static int32_t g_concurrentLimit = 32;
static int64_t g_cachedAuthorityBytes = 65536;

HttpClientOptions::HttpClientOptions() :
    m_timeoutMs(30000),
#ifndef __EMSCRIPTEN__
    m_maxRedirects(8),
#endif
    m_maxResponseBytes(16 * 1024 * 1024)
{

}

bool HttpClientOptions::setMaxResponseBytes(int64_t bytes)
{
    if (bytes != -1 && (bytes <= 0 || static_cast<uint64_t>(bytes) > UINT32_MAX))
    {
        return false;
    }
    m_maxResponseBytes = bytes == -1 ? UINT32_MAX : static_cast<uint32_t>(bytes);
    return true;
}

bool HttpClientOptions::setTimeoutMs(uint32_t milliseconds)
{
    if (milliseconds == 0 || milliseconds > INT32_MAX)
    {
        return false;
    }
    m_timeoutMs = milliseconds;
    return true;
}

#ifndef __EMSCRIPTEN__
bool HttpClientOptions::setMaxRedirects(int32_t redirects)
{
    if (redirects < -1)
    {
        return false;
    }
    m_maxRedirects = redirects;
    return true;
}
#endif

HttpClientResponse::HttpClientResponse() :
    m_status(0)
{

}

#ifndef __EMSCRIPTEN__
// 最后构造、最先销毁。关闭时先禁止新任务，再在锁外等待所有回调返回。
static HttpNativeWorkers g_nativeWorkers;

HttpNativeWorkers::HttpNativeWorkers() :
    m_stopping(false),
    m_initialized(false)
{

}

HttpNativeWorkers::~HttpNativeWorkers()
{
    stop();
}

void HttpNativeWorkers::stop()
{
    std::lock_guard<std::mutex> shutdown(m_shutdownMutex);
    std::vector<std::future<void>> workers;
    {
        std::lock_guard<std::mutex> lock(g_requestMutex);
        m_stopping = true;
        workers.swap(m_workers);
    }
    for (auto& worker : workers)
    {
        worker.wait();
    }
    closeTransport();
}

void HttpRequestClient::shutdown()
{
    g_nativeWorkers.stop();
}
#endif

bool HttpRequestClient::setLimits(int32_t concurrent, int64_t cachedAuthorityBytes)
{
    std::lock_guard<std::mutex> lock(g_requestMutex);
#ifndef __EMSCRIPTEN__
    if (g_nativeWorkers.m_stopping)
    {
        return false;
    }
#endif
    if (g_started || concurrent == 0 || concurrent < -1 || cachedAuthorityBytes < -1)
    {
        return false;
    }
    g_concurrentLimit = concurrent;
    g_cachedAuthorityBytes = cachedAuthorityBytes;
    return true;
}

/** 检查可选请求头的长度及 HTTP 字段值。
@param [in] value 请求字段值，允许水平制表和可见字节
@return 当前值可交给各端传输层时返回 true
*/
static bool ValidHeader(const std::string& value)
{
    if (value.size() > 8192)
    {
        return false;
    }
    for (size_t i = 0; i < value.size(); ++i)
    {
        const unsigned char ch = static_cast<unsigned char>(value[i]);
        if ((ch < 32 && ch != '\t') || ch == 127)
        {
            return false;
        }
    }
    return true;
}

/** 检查绝对 URL 的主机及可选十进制端口，保留路径和查询串中的标点。
@param [in] url 完整 HTTP 或 HTTPS 地址
@param [in] begin 协议及两个斜杠之后的首个位置
@return 主机非空、括号边界正确且可选端口在 0 到 65535 内时返回 true
*/
static bool ValidAuthority(const std::string& url, size_t begin)
{
    const size_t delimiter = url.find_first_of("/?#", begin);
    const size_t end = delimiter == std::string::npos ? url.size() : delimiter;
    if (begin == end || url.find('@', begin) < end) { return false; }

    size_t port;
    if (url[begin] == '[')
    {
        const size_t close = url.find(']', begin + 1);
        if (close >= end || close == begin + 1 || url.find('[', begin + 1) < close) { return false; }
        port = close + 1;
    }
    else
    {
        port = url.find_first_of(":[]", begin);
        if (port >= end) { return true; }
        if (port == begin) { return false; }
    }
    if (port == end) { return true; }
    if (url[port] != ':') { return false; }
    // 空端口沿用协议默认值。非空端口逐位限幅，避免长数字溢出。
    unsigned value = 0;
    for (++port; port < end; ++port)
    {
        const char digit = url[port];
        if (digit < '0' || digit > '9') { return false; }
        value = value * 10 + static_cast<unsigned>(digit - '0');
        if (value > 65535) { return false; }
    }
    return true;
}

/** 比较两个调用的全部请求选项。
@param [in] left 已经登记的选项
@param [in] right 当前调用的选项
@return 两组配置允许共享同一传输时返回 true
*/
static bool SameOptions(const HttpClientOptions& left, const HttpClientOptions& right)
{
#ifndef __EMSCRIPTEN__
    if (left.m_maxRedirects != right.m_maxRedirects) { return false; }
#endif
    return left.m_timeoutMs == right.m_timeoutMs && left.m_maxResponseBytes == right.m_maxResponseBytes &&
        left.m_authorityPem == right.m_authorityPem && left.m_range == right.m_range &&
        left.m_ifRange == right.m_ifRange && left.m_ifNoneMatch == right.m_ifNoneMatch && left.m_contentType == right.m_contentType;
}

void HttpRequestFinish(uint32_t id, const HttpClientResponse& response)
{
    std::vector<HttpRequestClient::Completion> callbacks;
    {
        std::lock_guard<std::mutex> lock(g_requestMutex);
        const auto it = g_requests.find(id);
        if (it == g_requests.end())
        {
            return;
        }
        callbacks.swap(it->second->m_callbacks);
        g_requests.erase(it);
    }
    for (size_t index = 0; index < callbacks.size(); ++index)
    {
        try
        {
            callbacks[index](response);
        }
        catch (...)
        {
            /* 各调用方独立接收结果。 */
        }
    }
}

/** 提交已登记的请求，传输层异常时通过原完成回调返回错误。
@param [in] work 已登记并持有输入与回调的请求
*/
static void RunRequest(const std::shared_ptr<HttpRequestWork>& work)
{
    try
    {
#ifdef __EMSCRIPTEN__
        HttpRequestSend(*work);
#else
        g_nativeWorkers.send(*work);
#endif
    }
    catch (...)
    {
        HttpClientResponse response;
        response.m_error = "HTTP transport failed";
        HttpRequestFinish(work->m_id, response);
    }
}

/** 校验并登记回调，复用在途请求或启动新的传输。
@param [in] post 是否发送 POST
@param [in] url HTTP 或 HTTPS 绝对地址
@param [in] body 请求正文，按字节保存
@param [in] options 本次请求使用的配置
@param [in] callback 请求结束时的完成通知
@return 请求或合并订阅已接纳时返回 true
*/
static bool SubmitRequest(bool post, const std::string& url, const std::string& body,
    const HttpClientOptions& options, const HttpRequestClient::Completion& callback)
{
#ifndef __EMSCRIPTEN__
    if (options.m_maxRedirects < -1) { return false; }
#endif
    const bool secure = url.compare(0, 8, "https://") == 0;
    if (!callback || (!secure && url.compare(0, 7, "http://") != 0) || url.size() > 8192 ||
        std::any_of(url.begin(), url.end(), [](unsigned char ch) { return ch <= 32 || ch == 127 || ch == '\\'; }) ||
        !options.m_timeoutMs ||
        options.m_timeoutMs > 0x7fffffff || !options.m_maxResponseBytes || body.size() > 0x7fffffff ||
        (!options.m_authorityPem.empty() && !secure) || !ValidHeader(options.m_range) ||
        !ValidHeader(options.m_ifRange) || !ValidHeader(options.m_ifNoneMatch) || !ValidHeader(options.m_contentType))
    {
        return false;
    }
    if (!ValidAuthority(url, secure ? 8 : 7))
    {
        return false;
    }
#ifdef __EMSCRIPTEN__
    if (!options.m_authorityPem.empty())
    {
        return false;
    }
#endif
    std::shared_ptr<HttpRequestWork> work;
    {
        std::lock_guard<std::mutex> lock(g_requestMutex);
#ifndef __EMSCRIPTEN__
        if (g_nativeWorkers.m_stopping)
        {
            return false;
        }
        auto& workers = g_nativeWorkers.m_workers;
        workers.erase(std::remove_if(workers.begin(), workers.end(), [](std::future<void>& job) {
            return job.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
        }), workers.end());
#endif
        for (const auto& entry : g_requests)
        {
            HttpRequestWork& pending = *entry.second;
            if (pending.m_post == post && pending.m_url == url && pending.m_body == body && SameOptions(pending.m_options, options))
            {
                pending.m_callbacks.push_back(callback);
                return true;
            }
        }
        if (g_requests.size() == UINT32_MAX ||
            (g_concurrentLimit > 0 && g_requests.size() >= static_cast<size_t>(g_concurrentLimit)))
        {
            return false;
        }
#ifndef __EMSCRIPTEN__
        if (g_concurrentLimit > 0 && workers.size() >= static_cast<size_t>(g_concurrentLimit))
        {
            return false;
        }
        workers.reserve(workers.size() + 1);
#endif
        work = std::make_shared<HttpRequestWork>();
        do { ++g_nextRequest; } while (g_nextRequest == 0 || g_requests.count(g_nextRequest));
        work->m_id = g_nextRequest;
        work->m_post = post;
        work->m_cacheAuthority = g_cachedAuthorityBytes < 0 ||
            options.m_authorityPem.size() <= static_cast<uint64_t>(g_cachedAuthorityBytes);
        work->m_url = url;
        work->m_body = body;
        work->m_options = options;
        work->m_callbacks.push_back(callback);
        g_requests.emplace(work->m_id, work);
#ifndef __EMSCRIPTEN__
        try
        {
            workers.push_back(std::async(std::launch::async, RunRequest, work));
        }
        catch (...)
        {
            g_requests.erase(work->m_id);
            return false;
        }
#endif
        g_started = true;
    }
#ifdef __EMSCRIPTEN__
    RunRequest(work);
#endif
    return true;
}

bool HttpRequestClient::httpGet(const std::string& url, const HttpClientOptions& options, const Completion& completion)
{
    return SubmitRequest(false, url, std::string(), options, completion);
}

std::string HttpRequestClient::httpPost(const std::string& url, const std::string& body,
    const HttpClientOptions& options, const Completion& completion)
{
    std::string id = CEncodeDecode::MD5Encode(url + body);
    return !id.empty() && SubmitRequest(true, url, body, options, completion) ? id : std::string();
}

// 下列示例已用 Windows x64 Release 单独编译运行。
// #include "HttpRequest/HttpRequestAPI.h"
// #include <cstdio>
// #include <functional>
// #include <string>
// #ifdef __EMSCRIPTEN__
// #include <emscripten.h>
// #else
// #include <future>
// #endif
//
// int main(int argc, char** argv)
// {
//     if (argc != 2) { return 2; }
//     const std::string origin(argv[1]);
//     HttpClientOptions options;
//     if (!HttpRequestClient::setLimits(8) || !options.setTimeoutMs(3000) ||
//         !options.setMaxResponseBytes(4096)) { return 1; }
// #ifdef __EMSCRIPTEN__
//     // Wasm 将控制权交还宿主。等回调正常返回后，再以最后一次请求的结果退出示例。
//     const std::function<void(bool)> done = [](bool ok) {
//         static int exitCode;
//         exitCode = ok ? 0 : 1;
//         std::puts(ok ? "HttpRequest demo PASS" : "HttpRequest demo FAIL");
//         emscripten_async_call([](void*) { emscripten_force_exit(exitCode); }, nullptr, 0);
//     };
// #else
//     if (!options.setMaxRedirects(2)) { return 1; }
//     std::promise<bool> completed;
//     std::future<bool> result = completed.get_future();
//     const std::function<void(bool)> done = [&completed](bool ok) { completed.set_value(ok); };
// #endif
//     const bool accepted = HttpRequestClient::httpGet(origin + "/keep-media", options,
//         [origin, options, done](const HttpClientResponse& first) mutable {
//             if (first.m_status != 200 || first.m_body != std::string("A\0BCD", 5)) { done(false); return; }
//             options.m_contentType = "application/octet-stream";
//             const std::string payload("a\0b", 3);
//             const std::string id = HttpRequestClient::httpPost(origin + "/echo", payload, options,
//                 [done, payload](const HttpClientResponse& reply) { done(reply.m_status == 200 && reply.m_body == payload); });
//             if (id.empty()) { done(false); }
//         });
// #ifdef __EMSCRIPTEN__
//     return accepted ? 0 : 1;
// #else
//     const bool ok = accepted && result.get();
//     // 原生所有者线程等待所有回调结束，随后允许捕获的对象离开作用域。
//     HttpRequestClient::shutdown();
//     return ok ? 0 : 1;
// #endif
// }