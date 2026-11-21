#include "HttpWorkTest.h"
#include "HttpService/HttpServiceAPI.h"
#include "civetweb/civetweb.h"
#include <chrono>
#include <cstring>
#include <cstdio>
#include <future>
#include <memory>

namespace
{
    // 真实处理器的进入和释放信号控制请求次序。
    class OptionsHandler : public HttpHandler
    {
    public:
        // 首个请求已经进入处理器
        std::promise<void> m_entered;
        // 测试线程允许首个请求返回
        std::promise<void> m_release;
        // 第二个独立连接已获工作线程调度
        std::promise<void> m_second;

        /** 用查询参数选取要触发的同步信号，返回可保持连接的单字节正文
        @param [in] request 实际解析的回环请求
        @return 允许复用连接的 X 正文
        */
        HttpResponse handle(const HttpRequest& request)
        {
            if (request.m_queryString == "first")
            {
                m_entered.set_value();
                m_release.get_future().wait_for(std::chrono::seconds(5));
            }
            else if (request.m_queryString == "second")
            {
                m_second.set_value();
            }
            HttpResponse result = HttpResponse::text("X");
            result.m_closeConnection = false;
            // 兼容调用方可直接赋值字段表。实际消息边界必须由发送器根据正文生成。
            result.m_headers["content-length"] = "8";
            result.m_headers["Connection"] = "close";
            result.m_headers["Transfer-Encoding"] = "identity";
            result.m_headers["X-Bad Field"] = "ignored";
            result.m_headers["X-Policy"] = "accepted";
            return result;
        }
    };

    typedef std::unique_ptr<mg_connection, void (*)(mg_connection*)> Client;

    /** 取得已连接的独占回环客户端
    @param [in] port 测试服务端口
    @return 成功时持有本次回环连接
    */
    Client Connect(uint16_t port)
    {
        char error[128] = {};
        return Client(mg_connect_client("127.0.0.1", port, 0, error, sizeof(error)), mg_close_connection);
    }

    /** 将完整 GET 或指定片段提交至当前连接
    @param [in,out] client 本次独占连接
    @param [in] request 以 NUL 结尾的请求文本
    @return 发送完整请求时报 true
    */
    bool Send(Client& client, const char* request)
    {
        const size_t size = std::strlen(request);
        return client && mg_write(client.get(), request, size) == static_cast<int>(size);
    }

    /** 读取一个完整的状态行及单字节正文
    @param [in,out] client 本次独占连接
    @param [in] keep 期望的连接保持策略
    @return 状态码为 200、连接和分帧字段正确且正文为 X 时返回 true
    */
    bool Received(Client& client, bool keep)
    {
        char error[128] = {}, body = 0;
        if (!client || mg_get_response(client.get(), error, sizeof(error), 3000) < 0) { return false; }
        const mg_response_info* info = mg_get_response_info(client.get());
        if (!info || info->status_code != 200) { return false; }
        unsigned lengths = 0, connections = 0, custom = 0;
        for (int i = 0; i < info->num_headers; ++i)
        {
            const std::string name(info->http_headers[i].name);
            const std::string value(info->http_headers[i].value);
            if (name == "Content-Length" && value == "1") { ++lengths; }
            if (name == "Connection" && value == (keep ? "keep-alive" : "close")) { ++connections; }
            if (name == "X-Policy" && value == "accepted") { ++custom; }
            if (name == "content-length" || name == "Transfer-Encoding" || name == "X-Bad Field" ||
                (name == "Connection" && value != (keep ? "keep-alive" : "close"))) { return false; }
        }
        return lengths == 1 && connections == 1 && custom == 1 && mg_read(client.get(), &body, 1) == 1 && body == 'X';
    }
}

bool CheckHttpServiceOptions(uint16_t port)
{
    std::shared_ptr<OptionsHandler> handler(new OptionsHandler);
    HttpService service;
    HttpServiceOptions options;
    options.m_listenHost = "127.0.0.1";
    options.m_port = port;
    options.m_workerThreads = 1;
    options.m_requestTimeoutMs = 1000;
    options.m_enableKeepAlive = true;
    if (!service.addHandler("/policy", handler) || !service.setOptions(options) || !service.start()) { return false; }
    Client first = Connect(port);
    if (!Send(first, "GET /policy?first HTTP/1.1\r\nHost: localhost\r\nConnection: keep-alive\r\n\r\n") ||
        handler->m_entered.get_future().wait_for(std::chrono::seconds(3)) != std::future_status::ready) { return false; }
    std::weak_ptr<OptionsHandler> active = handler;
    std::shared_ptr<OptionsHandler> replacement(new OptionsHandler);
    if (!service.addHandler("/policy", replacement)) { return false; }
    handler.reset();
    Client second = Connect(port);
    std::future<void> secondStarted = replacement->m_second.get_future();
    const bool queued = Send(second, "GET /policy?second HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n") &&
        secondStarted.wait_for(std::chrono::milliseconds(150)) == std::future_status::timeout;
    {
        std::shared_ptr<OptionsHandler> running = active.lock();
        if (!running) { return false; }
        running->m_release.set_value();
    }
    const bool gotFirst = Received(first, true);
    const bool reused = gotFirst && Send(first, "GET /policy HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n") &&
        Received(first, false);
    first.reset();
    const bool secondRan = secondStarted.wait_for(std::chrono::seconds(3)) == std::future_status::ready;
    const bool gotSecond = secondRan && Received(second, false);
    if (!queued || !reused || !gotSecond || !active.expired())
    {
        std::fprintf(stderr, "HttpService options queued=%d first=%d reused=%d secondEntered=%d secondRead=%d\n", queued, gotFirst, reused, secondRan, gotSecond);
        return false;
    }
    second.reset();
    // 故意只发送一个有界的不完整本地请求，观察服务自身的超时关闭。
    Client partial = Connect(port);
    if (!Send(partial, "GET /policy HTTP/1.1\r\nHost: localhost\r\n")) { return false; }
    const auto start = std::chrono::steady_clock::now();
    char error[128] = {};
    const int result = mg_get_response(partial.get(), error, sizeof(error), 7000);
    const auto elapsed = std::chrono::steady_clock::now() - start;
    const mg_response_info* info = mg_get_response_info(partial.get());
    std::fprintf(stdout, "HttpService request deadline result=%d status=%d elapsedMs=%lld\n", result, info ? info->status_code : 0, static_cast<long long>(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()));
    // CivetWeb 1.11 在已收到不完整报头且读超时后返回 400，某些平台直接关闭连接。
    return (result < 0 || (info && (info->status_code == 400 || info->status_code == 408))) &&
        elapsed >= std::chrono::milliseconds(700) && elapsed < std::chrono::seconds(5);
}