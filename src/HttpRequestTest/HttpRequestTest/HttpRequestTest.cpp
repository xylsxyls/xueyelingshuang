#include "HttpRequestTest.h"
#include "HttpRequest/HttpRequestAPI.h"
#include <atomic>
#include <chrono>
#include <stdexcept>
#ifndef __EMSCRIPTEN__
#include <thread>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef SOCKET SocketHandle;
static const SocketHandle kBadSocket = INVALID_SOCKET;
#else
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <unistd.h>
typedef int SocketHandle;
static const SocketHandle kBadSocket = -1;
#endif

/** 拥有一个测试套接字，作用域退出时关闭 */
struct TestSocket
{
    // 由当前对象独占的描述符
    SocketHandle fd;
    /** 接管新建或接收的套接字 */
    explicit TestSocket(SocketHandle socket) : fd(socket) {}
    /** 仅回收有效描述符 */
    ~TestSocket()
    {
        if (fd != kBadSocket)
        {
#ifdef _WIN32
            closesocket(fd);
#else
            close(fd);
#endif
        }
    }
    TestSocket(const TestSocket&) = delete;
    TestSocket& operator=(const TestSocket&) = delete;
};

/** 为本用例配对 Windows 网络运行库的启动和退出 */
struct TestNetwork
{
    // 本次可使用系统套接字
    bool ready;
    /** 初始化本平台网络环境 */
    TestNetwork() : ready(true)
    {
#ifdef _WIN32
        WSADATA wsa;
        ready = WSAStartup(MAKEWORD(2, 2), &wsa) == 0;
#endif
    }
    /** 释放本次引用 */
    ~TestNetwork()
    {
#ifdef _WIN32
        if (ready)
        {
            WSACleanup();
        }
#endif
    }
};

/** 设置读写上限，防止服务或测试源站停滞时永久等待
@param [in] fd 已创建的套接字
@return 成功返回 true
*/
static bool SetDeadline(SocketHandle fd)
{
#ifdef _WIN32
    int limit = 3500;
#else
    struct timeval limit = {3, 500000};
#ifdef SO_NOSIGPIPE
    int ignore = 1;
    setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &ignore, sizeof(ignore));
#endif
#endif
    return setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&limit), sizeof(limit)) == 0 &&
        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&limit), sizeof(limit)) == 0;
}

/** 完整写出短测试消息
@param [in] fd 连接的套接字
@param [in] text 请求或响应字节，可含 NUL
@return 成功返回 true
*/
static bool SendAll(SocketHandle fd, const std::string& text)
{
    size_t offset = 0;
    while (offset < text.size())
    {
#ifdef MSG_NOSIGNAL
        const int flags = MSG_NOSIGNAL;
#else
        const int flags = 0;
#endif
        const int sent = static_cast<int>(send(fd, text.data() + offset, static_cast<int>(text.size() - offset), flags));
        if (sent <= 0)
        {
            return false;
        }
        offset += sent;
    }
    return true;
}
#else
#include <emscripten.h>
#endif
#include "PlatformTest/PlatformTestAPI.h"
#include <cstdio>
#include <cstring>

#define CHECK(test) do { if (!(test)) { std::fprintf(stderr, "line %d: %s\n", __LINE__, #test); return 1; } } while (false)

// 仅测试 3 读写，各回调拥有独立的序号，结果在最后一个完成后可读
static std::atomic<int> g_pending(0);
static std::atomic<int> g_failures(0);
static std::atomic<unsigned> g_seen(0);
// 由命令行或设备宿主提供的可达测试源站
static std::string g_origin;

/** 核对选项边界及失败时的旧值
@return 成功返回 0
*/
static int Options()
{
    HttpClientOptions o;
    HttpClientResponse r;
    CHECK(r.m_status == 0 && r.m_error.empty() && r.m_body.empty());
    CHECK(o.m_timeoutMs == 30000 && o.m_maxResponseBytes == 16u * 1024 * 1024 && o.m_authorityPem.empty());
    CHECK(o.setTimeoutMs(1) && !o.setTimeoutMs(0) && o.m_timeoutMs == 1);
    CHECK(o.setTimeoutMs(INT32_MAX) && !o.setTimeoutMs(UINT32_MAX) && o.m_timeoutMs == INT32_MAX);
    CHECK(o.setMaxResponseBytes(1) && !o.setMaxResponseBytes(-2) && !o.setMaxResponseBytes(0) && o.m_maxResponseBytes == 1);
    CHECK(o.setMaxResponseBytes(-1) && o.m_maxResponseBytes == UINT32_MAX &&
        !o.setMaxResponseBytes(int64_t(UINT32_MAX) + 1) && o.m_maxResponseBytes == UINT32_MAX);
#ifndef __EMSCRIPTEN__
    CHECK(o.m_maxRedirects == 8 && o.setMaxRedirects(0) && o.setMaxRedirects(-1) &&
        !o.setMaxRedirects(-2) && o.m_maxRedirects == -1);
#endif
    return 0;
}

/** 拒绝不合法的调用，回调捕获对象不依赖用例栈的生命周期
@return 成功返回 0
*/
static int Reject()
{
    static std::atomic<int> unexpected(0);
    const HttpRequestClient::Completion notify = [](const HttpClientResponse&) { ++unexpected; };
    HttpClientOptions o;
    const std::string invalid[] = {"", "file:///sample", "http://", "http://host:65536/", std::string(8193, 'x')};
    for (const std::string& url : invalid)
    {
        CHECK(!HttpRequestClient::httpGet(url, o, notify) && HttpRequestClient::httpPost(url, "x", o, notify).empty());
    }
    const char* url = "http://127.0.0.1/";
    CHECK(!HttpRequestClient::httpGet(url, o, HttpRequestClient::Completion()));
    o.m_range = "bytes=1\nmore";
    CHECK(!HttpRequestClient::httpGet(url, o, notify));
    o.m_range.clear();
    o.m_maxResponseBytes = 0;
    CHECK(!HttpRequestClient::httpGet(url, o, notify));
    CHECK(!HttpRequestClient::setLimits(0) && !HttpRequestClient::setLimits(1, -2) && unexpected == 0);
    return 0;
}

#ifndef __EMSCRIPTEN__
/** 回环端口经 bind(0) 分配；只响应本用例的四个请求 */
struct LocalOrigin
{
    // 监听描述符
    TestSocket listener;

    /** 创建监听描述符 */
    LocalOrigin() : listener(socket(AF_INET, SOCK_STREAM, 0)) {}

    /** 绑定回环并返回绝对 URL
    @return 成功时为 URL，失败为空
    */
    std::string open()
    {
        sockaddr_in address = {};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
#ifdef _WIN32
        int length = sizeof(address);
#else
        socklen_t length = sizeof(address);
#endif
        if (listener.fd == kBadSocket || bind(listener.fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) ||
            listen(listener.fd, 8) || getsockname(listener.fd, reinterpret_cast<sockaddr*>(&address), &length))
        {
            return "";
        }
        return "http://127.0.0.1:" + std::to_string(ntohs(address.sin_port));
    }

    /** 在截止时间内处理一个请求，用真实收到的正文生成回显
    @return 成功返回 true
    */
    bool serve()
    {
#ifndef _WIN32
        if (listener.fd < 0 || listener.fd >= FD_SETSIZE)
        {
            return false;
        }
#endif
        fd_set active;
        FD_ZERO(&active);
        FD_SET(listener.fd, &active);
        timeval timeout = {6, 0};
#ifdef _WIN32
        const int nfds = 0;
#else
        const int nfds = listener.fd + 1;
#endif
        if (select(nfds, &active, nullptr, nullptr, &timeout) != 1)
        {
            return false;
        }
        TestSocket client(accept(listener.fd, nullptr, nullptr));
        if (client.fd == kBadSocket || !SetDeadline(client.fd))
        {
            return false;
        }
        std::string wire;
        size_t end = std::string::npos;
        bool post = false;
        while (end == std::string::npos || (post && wire.size() < end + 7))
        {
            char buf[1024];
            const int n = static_cast<int>(recv(client.fd, buf, sizeof(buf), 0));
            if (n <= 0 || wire.size() > 16384)
            {
                return false;
            }
            wire.append(buf, n);
            end = wire.find("\r\n\r\n");
            post = wire.compare(0, 5, "POST ") == 0;
        }
        const bool echo = wire.find("POST /echo ") == 0;
        const bool sample = wire.find("GET /video/sample ") == 0;
        const std::string data = echo ? wire.substr(end + 4, 3) : (sample ? std::string("A\0BCD", 5) : "missing");
        const std::string status = echo || sample ? "200 OK" : "404 Not Found";
        return SendAll(client.fd, "HTTP/1.1 " + status + "\r\nConnection: close\r\nContent-Length: " +
            std::to_string(data.size()) + "\r\n\r\n" + data);
    }
};
#endif

/** 取得用于检查某一次传输的完成回调
@param [in] id 本轮独立请求编号 0 至 3
@return 生命周期独立于调用者的回调对象
*/
static HttpRequestClient::Completion Observe(unsigned id)
{
    return [id](const HttpClientResponse& response)
    {
        bool correct;
        if (id == 3)
        {
            correct = !response.m_error.empty();
        }
        else
        {
            const std::string expected[] = {std::string("A\0BCD", 5), std::string("x\0z", 3), "missing"};
            correct = response.m_error.empty() && response.m_body == expected[id] &&
                response.m_status == (id == 2 ? 404 : 200);
        }
        if (!correct || (g_seen.fetch_or(1u << id) & (1u << id)))
        {
            ++g_failures;
        }
        --g_pending;
    };
}

/** 发起四个独立传输，对已接纳的请求逐项判定最终回调
@return 原生完成或 Wasm 已调度返回 0，最终 Wasm 结果由宿主轮询
*/
static int Transfer()
{
    std::string origin = g_origin;
#ifndef __EMSCRIPTEN__
    TestNetwork network;
    CHECK(network.ready);
    LocalOrigin local;
    if (origin.empty())
    {
        origin = local.open();
    }
#endif
    if (origin.empty())
    {
        std::fprintf(stderr, "HTTP transfer requires a reachable fixture via --origin URL\n");
        return 1;
    }
    CHECK(HttpRequestClient::setLimits(4));
    HttpClientOptions o;
    CHECK(o.setTimeoutMs(5000));
    g_pending = 4;
    CHECK(HttpRequestClient::httpGet(origin + "/video/sample", o, Observe(0)));
    const std::string key = HttpRequestClient::httpPost(origin + "/echo", std::string("x\0z", 3), o, Observe(1));
    CHECK(key.size() == 32 && key.find_first_not_of("0123456789abcdef") == std::string::npos);
    CHECK(HttpRequestClient::httpGet(origin + "/missing", o, Observe(2)));
    CHECK(o.setMaxResponseBytes(2) && HttpRequestClient::httpGet(origin + "/video/sample", o, Observe(3)));
    CHECK(!HttpRequestClient::setLimits(8));
#ifndef __EMSCRIPTEN__
    if (g_origin.empty())
    {
        for (unsigned i = 0; i < 4; ++i)
        {
            CHECK(local.serve());
        }
    }
    const auto end = std::chrono::steady_clock::now() + std::chrono::seconds(7);
    while (g_pending > 0 && std::chrono::steady_clock::now() < end)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    HttpRequestClient::shutdown();
    CHECK(g_pending == 0 && g_seen == 15u && g_failures == 0);
    CHECK(!HttpRequestClient::httpGet(origin, o, Observe(0)));
#endif
    return 0;
}

#ifdef __EMSCRIPTEN__
/** 宿主事件循环轮询，确保没有未完成的传输被计为通过
@return -1 等待，0 全部成功，1 失败
*/
extern "C" EMSCRIPTEN_KEEPALIVE int httpRequestTestResult()
{
    if (g_pending > 0)
    {
        return -1;
    }
    return g_pending == 0 && g_failures == 0 && (g_seen == 0u || g_seen == 15u) ? 0 : 1;
}
#endif

/** 计量提交被同步拒绝时的成本
@return 成功返回 0
*/
static int Benchmark()
{
    HttpClientOptions o;
    const HttpRequestClient::Completion callback = [](const HttpClientResponse&) {};
    const std::string url("file:///invalid");
    LibraryBenchmark::run("httpRequest.reject.scheme", [&](uint32_t count) -> uint64_t
    {
        uint64_t rejected = 0;
        for (uint32_t i = 0; i < count; ++i)
        {
            rejected += !HttpRequestClient::httpGet(url, o, callback);
        }
        if (rejected != count)
        {
            throw std::runtime_error("invalid scheme");
        }
        return rejected;
    });
    return 0;
}

/** 解析可达源站及公共用例选择参数
@param [in] argc 命令行参数个数
@param [in] argv 用例选项及对应参数
@return 全部选定用例通过时返回 0，失败返回非零
*/
int main(int argc, char** argv)
{
    if (argc >= 3 && std::strcmp(argv[1], "--origin") == 0)
    {
        g_origin = argv[2];
        argc -= 2;
        argv += 2;
    }
    if (argc == 2 && std::strcmp(argv[1], "--benchmark") == 0)
    {
        return Benchmark();
    }
    const LibraryTestCase cases[] = {{1, false, Options}, {2, false, Reject}, {3, true, Transfer}};
    return LibraryCaseSelection::run(argc, argv, cases);
}