#include "HttpServiceTest.h"
#include "HttpService/HttpServiceAPI.h"
#include "civetweb/civetweb.h"
#include <chrono>
#include <memory>
#include <stdexcept>
#ifdef _MSC_VER
#ifdef _DEBUG
#pragma comment(lib, "civetwebd.lib")
#else
#pragma comment(lib, "civetweb.lib")
#endif
#pragma comment(lib, "ws2_32.lib")
#endif
#include "PlatformTest/PlatformTestAPI.h"
#include <cstdio>
#include <cstring>

#define CHECK(test) do { if (!(test)) { std::fprintf(stderr, "line %d: %s\n", __LINE__, #test); return 1; } } while (false)

/** 处理器通过响应正文反映接收的查询、正文及路由版本 */
class EchoHandler : public HttpHandler
{
public:
    /** 创建具有指定版本标识的处理器 */
    explicit EchoHandler(const std::string& tag) : m_tag(tag) {}

    /** 返回当前请求的独立观测值 */
    HttpResponse handle(const HttpRequest& request) override
    {
        auto it = request.m_query.find("name");
        return HttpResponse::text(m_tag + "|" + request.m_method + "|" + request.m_uri + "|" +
            (it == request.m_query.end() ? "" : it->second) + "|" + request.m_body);
    }
private:
    // 回包中可验证的当前路由版本
    std::string m_tag;
};

/** 核对响应体的长度语义及附加头规则
@return 成功返回 0
*/
static int ResponseValues()
{
    HttpRequest req;
    CHECK(!req.m_isSsl && req.m_contentLength == 0 && req.m_remotePort == 0);
    HttpResponse initial;
    CHECK(initial.m_statusCode == 200 && initial.m_statusText == "OK");
    const std::string bytes("a\0b", 3);
    HttpResponse text = HttpResponse::text(bytes, 201);
    CHECK(text.m_body == bytes && text.m_contentType.find("text/plain") == 0 && text.m_statusCode == 201);
    HttpResponse json = HttpResponse::json("{}", 202);
    CHECK(json.m_body == "{}" && json.m_contentType.find("application/json") == 0);
    text.setStatus(404);
    CHECK(text.m_statusCode == 404 && text.m_statusText == "Not Found");
    text.setHeader("X-Note", "first");
    text.setHeader("x-note", "updated");
    text.setHeader("bad name", "ignored");
    text.setHeader("X-Bad", "bad\nvalue");
    CHECK(text.m_headers.size() == 1 && text.m_headers.at("X-Note") == "updated");
    return 0;
}

/** 验证启动前的配置和路由拥有期
@return 成功返回 0
*/
static int RouteOptions()
{
    HttpService service;
    HttpServiceOptions options;
    CHECK(options.m_port == 0 && options.m_maxRequestBodyBytes == 1048576 && !service.isRunning());
    CHECK(!service.start(options) && !service.lastError().empty());
    options.m_port = 23456;
    options.m_listenHost = "127.0.0.1";
    options.m_maxRequestBodyBytes = -2;
    CHECK(!service.setOptions(options));
    options.m_maxRequestBodyBytes = -1;
    CHECK(service.setOptions(options));
    std::shared_ptr<HttpHandler> h(new EchoHandler("first"));
    CHECK(!service.addHandler("", h) && !service.addHandler("/empty", static_cast<HttpHandler*>(nullptr)));
    CHECK(service.addHandler("/route", h));
    std::weak_ptr<HttpHandler> old(h);
    h.reset(new EchoHandler("second"));
    CHECK(!old.expired() && service.addHandler("/route", h) && old.expired());
    old = h;
    h.reset();
    service.clearHandlers();
    CHECK(old.expired() && !service.isRunning());
    return 0;
}

/** 用独立连接写入请求并读取 Content-Length 指定的正文
@param [in] port 本用例成功绑定的回环端口
@param [in] request 完整 HTTP 请求
@param [in] expectedStatus 应返回的 HTTP 状态
@param [out] body 接收响应字节
@return 状态与完整长度匹配时返回 true
*/
static bool Exchange(uint16_t port, const std::string& request, int expectedStatus, std::string& body)
{
    char error[128] = {};
    std::unique_ptr<mg_connection, decltype(&mg_close_connection)> connection(
        mg_connect_client("127.0.0.1", port, 0, error, sizeof(error)), &mg_close_connection);
    if (!connection || mg_write(connection.get(), request.data(), request.size()) != static_cast<int>(request.size()) ||
        mg_get_response(connection.get(), error, sizeof(error), 3000) < 0)
    {
        return false;
    }
    const mg_response_info* info = mg_get_response_info(connection.get());
    if (!info || info->status_code != expectedStatus || info->content_length < 0 || info->content_length > 4096)
    {
        return false;
    }
    body.clear();
    while (body.size() < static_cast<size_t>(info->content_length))
    {
        char chunk[512];
        const int n = mg_read(connection.get(), chunk, sizeof(chunk));
        if (n <= 0)
        {
            return false;
        }
        body.append(chunk, n);
    }
    return body.size() == static_cast<size_t>(info->content_length);
}

/** 验证真正的回环请求处理、长度上限和热替换
@return 成功返回 0
*/
static int Loopback()
{
    HttpService service;
    CHECK(service.addHandler("/inspect", std::make_shared<EchoHandler>("one")));
    HttpServiceOptions options;
    options.m_listenHost = "127.0.0.1";
    options.m_workerThreads = 2;
    options.m_requestTimeoutMs = 2500;
    options.m_maxRequestBodyBytes = 3;
    const unsigned seed = static_cast<unsigned>(std::chrono::steady_clock::now().time_since_epoch().count());
    for (unsigned i = 0; i < 40 && !service.isRunning(); ++i)
    {
        options.m_port = static_cast<uint16_t>(20000 + (seed + i) % 35000);
        service.start(options);
    }
    CHECK(service.isRunning() && service.start() && !service.setOptions(options));
    const std::string data("a\0b", 3);
    std::string body;
    const std::string post = "POST /inspect?name=A%20B HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\nContent-Length: ";
    CHECK(Exchange(options.m_port, post + "3\r\n\r\n" + data, 200, body) && body == "one|POST|/inspect|A B|" + data);
    CHECK(Exchange(options.m_port, post + "4\r\n\r\nxxxx", 413, body));
    CHECK(service.addHandler("/inspect", std::make_shared<EchoHandler>("two")));
    CHECK(Exchange(options.m_port, "GET /inspect HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n", 200, body) &&
        body == "two|GET|/inspect||");
    service.stop();
    service.stop();
    CHECK(!service.isRunning());
    return 0;
}

/** 计量真实响应对象的构造与正文复制
@return 成功返回 0
*/
static int Benchmark()
{
    const std::string body(256, 'a');
    LibraryBenchmark::run("httpService.responseText.256", [&](uint32_t count) -> uint64_t
    {
        uint64_t bytes = 0;
        for (uint32_t i = 0; i < count; ++i)
        {
            bytes += HttpResponse::text(body).m_body.size();
        }
        if (bytes != uint64_t(count) * body.size())
        {
            throw std::runtime_error("response length");
        }
        return bytes;
    });
    return 0;
}

/** 根据稳定编号运行功能用例，--benchmark 单独计时
@param [in] argc 命令行参数个数
@param [in] argv 用例选项及对应参数
@return 全部选定用例通过时返回 0，失败返回非零
*/
int main(int argc, char** argv)
{
    if (argc == 2 && std::strcmp(argv[1], "--benchmark") == 0)
    {
        return Benchmark();
    }
    const LibraryTestCase cases[] = {{1, false, ResponseValues}, {2, false, RouteOptions}, {3, true, Loopback}};
    return LibraryCaseSelection::run(argc, argv, cases);
}