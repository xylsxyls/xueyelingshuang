#include "HttpClientCases.h"
#include <chrono>
#include <cstdio>
#include <fstream>
#include <future>
#include <iterator>
#include <stdexcept>

/** 提交实际请求并等待该次独立回调，超时后回调仍独占其承诺对象
@param [in] origin 测试源站
@param [in] options 本次请求的头及信任配置
@param [in] post 是否发送包含空字节的 POST 正文
@param [in] path 本次访问的测试端点
@return 实际响应或传输错误
*/
static HttpClientResponse Request(const std::string& origin, const HttpClientOptions& options, bool post, const char* path = "/reuse")
{
    std::shared_ptr<std::promise<HttpClientResponse>> done(new std::promise<HttpClientResponse>);
    std::future<HttpClientResponse> result = done->get_future();
    HttpRequestClient::Completion complete = [done](const HttpClientResponse& value) { done->set_value(value); };
    const std::string url = origin + path;
    bool accepted = post ? !HttpRequestClient::httpPost(url, std::string("X\0Y", 3), options, complete).empty() :
        HttpRequestClient::httpGet(url, options, complete);
    if (!accepted || result.wait_for(std::chrono::seconds(8)) != std::future_status::ready)
    {
        throw std::runtime_error("Client did not complete its own request");
    }
    return result.get();
}

/** 检查源站端口、方法、头字段和正文，读取源站分配的实际连接号
@param [in] origin 接受请求的源站
@param [in] response 实际返回的响应
@param [in] expected 期望的方法、请求头和正文
@return 同一源站中唯一的端口与连接编号
*/
static std::string ConnectionId(const std::string& origin, const HttpClientResponse& response, const std::string& expected)
{
    const size_t newline = response.m_body.find('\n');
    const std::string prefix = origin.substr(origin.rfind(':') + 1) + "/";
    if (response.m_status != 200 || !response.m_error.empty() || newline == std::string::npos ||
        response.m_body.compare(0, prefix.size(), prefix) != 0 || response.m_body.substr(newline + 1) != expected)
    {
        throw std::runtime_error("Wrong connection origin, method, headers, body or completion");
    }
    return response.m_body.substr(0, newline);
}

int HttpClientReuseCases(int argc, char* argv[])
{
    if (argc < 4 || argc > 6)
    {
        return 2;
    }
    int result = 1;
    try
    {
        const std::string origin(argv[2]);
        const std::string other(argv[3]);
        HttpClientOptions options;
        options.m_timeoutMs = 5000;
        const std::string mode(argv[1]);
        if (mode == "--large-ca-default" || mode == "--large-ca-unlimited")
        {
            if (argc != 4)
            {
                return 2;
            }
            const bool unlimited = mode == "--large-ca-unlimited";
            std::ifstream ca(argv[3], std::ios::binary);
            options.m_authorityPem.assign(std::istreambuf_iterator<char>(ca), std::istreambuf_iterator<char>());
            if (options.m_authorityPem.empty() || options.m_authorityPem.size() > 65536 ||
                (unlimited && !HttpRequestClient::setLimits(32, -1))) { return 2; }
            // PEM 的纯空白尾部不改变受信根，跨越默认的 65536 字节私有材料缓存阈值。
            options.m_authorityPem.resize(65537, '\n');
            const std::string expected("GET\n\n\n\n\n");
            const std::string first = ConnectionId(origin, Request(origin, options, false), expected);
            const std::string second = ConnectionId(origin, Request(origin, options, false), expected);
            HttpRequestClient::shutdown();
            if ((first == second) != unlimited)
            {
                return 1;
            }
            std::printf("HttpWorkTest large CA PASS bytes=65537 cached=%d\n", unlimited ? 1 : 0);
            return 0;
        }
        if (argc >= 5)
        {
            std::ifstream ca(argv[4], std::ios::binary);
            options.m_authorityPem.assign(std::istreambuf_iterator<char>(ca), std::istreambuf_iterator<char>());
            if (options.m_authorityPem.empty())
            {
                throw std::runtime_error("Missing test CA");
            }
        }
        const bool bypassCache = std::string(argv[1]) == "--uncached-ca";
        if (bypassCache && (options.m_authorityPem.empty() || !HttpRequestClient::setLimits(32, 0)))
        {
            throw std::runtime_error("Missing bounded private CA case");
        }
        HttpClientOptions redirect = options;
        if (redirect.m_maxRedirects != 8 || redirect.setMaxRedirects(-2) || redirect.m_maxRedirects != 8)
        {
            throw std::runtime_error("Invalid redirect setter changed default");
        }
        redirect.m_maxRedirects = -2;
        if (HttpRequestClient::httpGet(origin, redirect, [](const HttpClientResponse&) {}))
        {
            throw std::runtime_error("Negative redirect limit accepted");
        }
        if (!redirect.setMaxRedirects(0) || Request(origin, redirect, false, "/redirect").m_error.empty())
        {
            throw std::runtime_error("Zero redirect budget was ignored");
        }
        if (!redirect.setMaxRedirects(1) || Request(origin, redirect, false, "/redirect").m_body != std::string("A\0BCD", 5) ||
            !redirect.setMaxRedirects(-1) || Request(origin, redirect, false, "/redirect").m_body != std::string("A\0BCD", 5))
        {
            throw std::runtime_error("Single redirect budget was ignored");
        }
        HttpClientOptions custom = options;
        custom.m_range = "bytes=1-3";
        custom.m_ifRange = "\"etag\"";
        custom.m_ifNoneMatch = "\"old\"";
        custom.m_contentType = "application/x-http-test";
        const std::string post = std::string("POST\nbytes=1-3\n\"etag\"\n\"old\"\napplication/x-http-test\n") + std::string("X\0Y", 3);
        const std::string get = "GET\n\n\n\n\n";
        const std::string first = ConnectionId(origin, Request(origin, custom, true), post);
        const std::string second = ConnectionId(origin, Request(origin, options, false), get);
        if (bypassCache ? first == second : first != second)
        {
            throw std::runtime_error("Unexpected connection identity for the configured CA cache limit");
        }
        if (argc >= 5)
        {
            HttpClientOptions system;
            system.m_timeoutMs = 5000;
            const HttpClientResponse refusal = Request(origin, system, true);
            if (refusal.m_error.empty() || refusal.m_status != 0)
            {
                throw std::runtime_error("A private trusted connection crossed the system trust boundary");
            }
            if (argc == 6)
            {
                std::ifstream otherCa(argv[5], std::ios::binary);
                system.m_authorityPem.assign(std::istreambuf_iterator<char>(otherCa), std::istreambuf_iterator<char>());
                if (system.m_authorityPem.empty() || system.m_authorityPem == options.m_authorityPem)
                {
                    throw std::runtime_error("Missing independent CA");
                }
                const HttpClientResponse wrong = Request(origin, system, true);
                if (wrong.m_error.empty() || wrong.m_status != 0)
                {
                    throw std::runtime_error("A trusted connection crossed two different private CAs");
                }
            }
            ConnectionId(origin, Request(origin, options, false), get);
        }
        HttpClientOptions plain;
        plain.m_timeoutMs = 5000;
        ConnectionId(other, Request(other, plain, false), get);
        const std::string returned = ConnectionId(origin, Request(origin, options, false), get);
        if (returned == first)
        {
            throw std::runtime_error("Evicted origin still retained an idle connection");
        }
        std::printf("HttpWorkTest reuse PASS first=%s next=%s trust=%s\n", first.c_str(), returned.c_str(), argc == 6 ? "private/system/other-private" : (argc == 5 ? "private/system" : "plain"));
        result = 0;
    }
    catch (const std::exception& error)
    {
        std::fprintf(stderr, "HttpWorkTest reuse FAIL %s\n", error.what());
    }
    HttpRequestClient::shutdown();
    return result;
}