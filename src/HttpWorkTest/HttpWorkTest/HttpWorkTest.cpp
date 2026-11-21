#include "HttpWorkTest.h"
#include "HttpClientCases.h"
#ifdef HTTPWORK_CLIENT_ONLY
int main(int argc, char* argv[])
{
    return HttpClientCasesMain(argc, argv);
}
#else
#include "HttpService/HttpServiceAPI.h"
#include "HttpService/HttpServiceHelper.h"
#include "HttpService/HandlerContext.h"

#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "civetweb/civetweb.h"
#include "PlatformTest/PlatformTestAPI.h"
#include <atomic>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <thread>
#ifdef _WIN32
#include <process.h>
#else
#include <cerrno>
#include <spawn.h>
#include <sys/wait.h>
extern char** environ;
#endif

/** 查找逗号分隔的 CORS 响应字段名，忽略 ASCII 大小写和两端 OWS。
@param [in] value 实际响应中的字段列表，可为空
@param [in] wanted 预期的小写字段名
@return 列表含有完整字段名时返回 true
*/
static bool CorsField(const char* value, const std::string& wanted)
{
    std::istringstream fields(value ? value : "");
    std::string token;
    while (std::getline(fields, token, ','))
    {
        const size_t begin = token.find_first_not_of(" \t");
        if (begin == std::string::npos) { continue; }
        token = token.substr(begin, token.find_last_not_of(" \t") - begin + 1);
        for (char& c : token)
        {
            if (c >= 'A' && c <= 'Z') { c = static_cast<char>(c + 'a' - 'A'); }
        }
        if (token == wanted) { return true; }
    }
    return false;
}

/** 核对跨源媒体请求的预检与实际响应，两个步骤分别通过回环连接读取。
@param [in] port 正在运行的测试服务端口
@param [in] path 注册的路由
@param [in] preflight true 验证允许的请求头，false 验证可读取的响应头
@param [in] origin 预期允许的源，空指针要求所有 CORS 字段均缺省
@return HTTP 状态及两个请求阶段各自需要的字段均正确时返回 true
*/
static bool CheckMediaCors(uint16_t port, const char* path, bool preflight, const char* origin)
{
    char error[128] = {};
    const char* request = preflight ?
        "OPTIONS %s HTTP/1.1\r\nHost: localhost\r\nOrigin: https://media.example.test\r\n"
        "Access-Control-Request-Method: GET\r\nAccess-Control-Request-Headers: range, if-range, if-none-match\r\n"
        "Connection: close\r\n\r\n" :
        "GET %s HTTP/1.1\r\nHost: localhost\r\nOrigin: https://media.example.test\r\nConnection: close\r\n\r\n";
    std::unique_ptr<mg_connection, void (*)(mg_connection*)> client(
        mg_download("127.0.0.1", port, 0, error, sizeof(error), request, path), mg_close_connection);
    if (!client) { return false; }
    const mg_response_info* info = mg_get_response_info(client.get());
    if (!info || info->status_code != (preflight ? 204 : 200)) { return false; }
    const char* fields = mg_get_header(client.get(), preflight ? "Access-Control-Allow-Headers" : "Access-Control-Expose-Headers");
    const char* allowed = mg_get_header(client.get(), "Access-Control-Allow-Origin");
    if (!origin)
    {
        return !allowed && !fields && !mg_get_header(client.get(), "Access-Control-Allow-Methods") &&
            !mg_get_header(client.get(), "Access-Control-Expose-Headers");
    }
    if (!allowed || std::string(allowed) != origin) { return false; }
    if (preflight && (!CorsField(mg_get_header(client.get(), "Allow"), "head") ||
        !CorsField(mg_get_header(client.get(), "Access-Control-Allow-Methods"), "head"))) { return false; }
    return preflight ? CorsField(fields, "range") && CorsField(fields, "if-range") && CorsField(fields, "if-none-match") :
        CorsField(fields, "content-range") && CorsField(fields, "etag");
}

/** 从实际程序路径启动一个独立的服务端子进程，等待其回环检查返回。
@param [in] path 可执行文件绝对 UTF-8 路径
@param [in] port 子进程的首选回环端口
@return 子进程通过所有检查时返回 true
*/
static bool CheckChildService(const std::string& path, uint16_t port)
{
    if (path.empty()) { return false; }
    const std::string number = CStringManager::toStringInt32(port);
#ifdef _WIN32
    const std::wstring program = CStringManager::Utf8ToUnicode(path);
    const std::wstring argument = L"\"" + program + L"\"";
    const std::wstring portText(number.begin(), number.end());
    const wchar_t* args[] = { argument.c_str(), L"--httpservice-child", portText.c_str(), nullptr };
    return _wspawnv(_P_WAIT, program.c_str(), args) == 0;
#else
    char mode[] = "--httpservice-child";
    char* args[] = { const_cast<char*>(path.c_str()), mode, const_cast<char*>(number.c_str()), nullptr };
    pid_t child;
    if (posix_spawn(&child, path.c_str(), nullptr, nullptr, args, environ) != 0) { return false; }
    int status;
    pid_t finished;
    do { finished = waitpid(child, &status, 0); } while (finished < 0 && errno == EINTR);
    return finished == child && WIFEXITED(status) && WEXITSTATUS(status) == 0;
#endif
}

/** 配置独立的回环服务，执行选定路由、并发或基准场景
@param [in] argc 真实命令行数量，选择器调用时可以为零
@param [in] argv 命令行，供子进程及基准模式取得入口
@param [in] functional 执行协议、路由、设置与资源回收场景
@param [in] pressure 执行多线程、多进程场景
@return 0 表示已选断言通过
*/
static int RunServiceCases(int argc, char* argv[], bool functional, bool pressure)
{
    int32_t totalCount = 0;
    int32_t failCount = 0;
    std::function<void(bool, const std::string&)> check = [&totalCount, &failCount](bool ok, const std::string& name) -> void
    {
        ++totalCount;
        std::cout << (ok ? "[PASS] " : "[FAIL] ") << name << std::endl;
        if (!ok)
        {
            ++failCount;
        }
    };
    unsigned firstPort = 58207;
    const char* portText = std::getenv("HTTPSERVICE_TEST_PORT");
    if (portText != nullptr)
    {
        if (!LibraryCaseSelection::number(portText, 65535, firstPort) || *portText != '\0') { return 2; }
    }
    std::function<bool(uint16_t, const std::string&, int32_t&, std::string&)> httpRequestByPort =
        [](uint16_t port, const std::string& requestText, int32_t& statusCode, std::string& body) -> bool
    {
        char error[256] = {};
        struct mg_connection* conn = mg_download("127.0.0.1", port, 0, error, sizeof(error), "%s", requestText.c_str());
        if (conn == nullptr)
        {
            body = error;
            statusCode = 0;
            return false;
        }
        const struct mg_response_info* responseInfo = mg_get_response_info(conn);
        statusCode = responseInfo == nullptr ? 0 : responseInfo->status_code;
        char buffer[1024] = {};
        int32_t readSize = 0;
        body.clear();
        while ((readSize = mg_read(conn, buffer, sizeof(buffer))) > 0)
        {
            body.append(buffer, static_cast<size_t>(readSize));
        }
        mg_close_connection(conn);
        return true;
    };

    class TestHandler : public HttpHandler
    {
    public:
        TestHandler() :
        m_count(0)
        {

        }

        virtual HttpResponse handle(const HttpRequest& request)
        {
            ++m_count;
            if (request.m_method == "GET")
            {
                if (request.m_queryString == "unchanged") { return HttpResponse::text("representation", 304); }
                if (request.m_queryString == "reset") { return HttpResponse::text("ignored", 205); }
                std::map<std::string, std::string>::const_iterator it = request.m_query.find("name");
                std::string name = it == request.m_query.end() ? "" : it->second;
                return HttpResponse::text("GET:" + name);
            }
            if (request.m_method == "POST")
            {
                return HttpResponse::json("{\"method\":\"POST\",\"body\":\"" + request.m_body + "\"}");
            }
            return HttpResponse::text("method not allowed", kHttpStatusMethodNotAllowed);
        }

        std::atomic<int32_t> m_count;
    };

    class ThrowHandler : public HttpHandler
    {
    public:
        virtual HttpResponse handle(const HttpRequest& request)
        {
            (void)request;
            throw 1;
        }
    };

    class ReplaceHandler : public HttpHandler
    {
    public:
        explicit ReplaceHandler(const std::string& text) :
        m_text(text)
        {

        }

        virtual HttpResponse handle(const HttpRequest& request)
        {
            (void)request;
            return HttpResponse::text(m_text);
        }

        std::string m_text;
    };

    class LargeHandler : public HttpHandler
    {
    public:
        virtual HttpResponse handle(const HttpRequest& request)
        {
            (void)request;
            return HttpResponse::text(std::string(200000, 'L'));
        }
    };

    class LegacyInterface : public HttpInterface
    {
    public:
        virtual bool httpGetReceive(const std::string& request)
        {
            writeSuccess();
            if (request == "status")
            {
                setStatus(202, "Queued");
                setHeader("X-Mode", "async");
            }
            writeResult("LEGACY_GET:" + request);
            return true;
        }

        virtual bool httpPostReceive(const std::string& request)
        {
            writeSuccess();
            writeResult("LEGACY_POST:" + request);
            return true;
        }
    };

    if (argc >= 3 && argv != nullptr && argv[1] != nullptr && std::string(argv[1]) == "--httpservice-child")
    {
        char* end = nullptr;
        const long childPortValue = std::strtol(argv[2], &end, 10);
        if (end == argv[2] || *end != 0 || childPortValue <= 0 || childPortValue > 65535)
        {
            return 2;
        }

        std::shared_ptr<TestHandler> childHandler(new TestHandler);
        HttpService childService;
        if (!childService.addHandler("/child", childHandler))
        {
            return 3;
        }
        HttpServiceOptions childOptions;
        childOptions.m_listenHost = "127.0.0.1";
        childOptions.m_workerThreads = 4;
        childOptions.m_enableKeepAlive = false;
        childOptions.m_enableCors = true;
        childOptions.m_requestTimeoutMs = 3000;
        childOptions.m_maxRequestBodyBytes = 128;
        bool childStarted = false;
        for (int32_t offset = 0; offset < 32 && childPortValue + offset <= 65535; ++offset)
        {
            childOptions.m_port = static_cast<uint16_t>(childPortValue + offset);
            if (childService.start(childOptions))
            {
                childStarted = true;
                break;
            }
        }
        if (!childStarted)
        {
            return 4;
        }

        bool childOk = true;
        for (int32_t i = 0; i < 10; ++i)
        {
            int32_t childStatusCode = 0;
            std::string childBody;
            if (!httpRequestByPort(childOptions.m_port,
                "GET /child?name=process HTTP/1.0\r\nHost: 127.0.0.1\r\n\r\n",
                childStatusCode,
                childBody) ||
                childStatusCode != kHttpStatusOk ||
                childBody != "GET:process")
            {
                childOk = false;
                break;
            }
        }
        childService.clearHandlers();
        return childOk ? 0 : 5;
    }

    auto corsOrigin = [](uint16_t port, const char* path) -> std::string
    {
        char error[256] = {};
        mg_connection* conn = mg_download("127.0.0.1", port, 0, error, sizeof(error),
            "GET %s HTTP/1.0\r\nHost: 127.0.0.1\r\n\r\n", path);
        if (!conn) { return "failed to connect"; }
        const mg_response_info* info = mg_get_response_info(conn);
        std::string result = "missing";
        if (!info || info->status_code != 200) { result = "invalid status"; }
        else for (int i = 0; i < info->num_headers; ++i)
        {
            if (std::string(info->http_headers[i].name) == "Access-Control-Allow-Origin")
            {
                result = info->http_headers[i].value;
                break;
            }
        }
        mg_close_connection(conn);
        return result;
    };

    if (functional)
    {
        HttpRequest request;
        check(request.m_method.empty() && request.m_remotePort == 0 && request.m_contentLength == 0, "HttpRequest default value");
        HttpResponse okResponse = HttpResponse::json("{\"ok\":true}");
        check(okResponse.m_statusCode == kHttpStatusOk && okResponse.m_contentType.find("json") != std::string::npos, "HttpResponse json");
        HttpResponse textResponse = HttpResponse::text("hello", kHttpStatusCreated);
        check(textResponse.m_statusCode == kHttpStatusCreated && textResponse.m_body == "hello", "HttpResponse text");
        textResponse.setHeader("X-Test", "1");
        check(textResponse.m_headers["X-Test"] == "1", "HttpResponse setHeader");
        check(HttpServiceHelper::hasHeader(textResponse.m_headers, "x-test"), "HttpServiceHelper hasHeader ignore case");
        textResponse.setHeader("x-TEST", "second");
        textResponse.setStatus(kHttpStatusNotFound);
        check(textResponse.m_statusCode == kHttpStatusNotFound && textResponse.m_statusText == "Not Found",
            "HttpResponse status resets standard reason text");
        check(HttpResponse::text("part", 206).m_statusText == "Partial Content" &&
            HttpResponse::defaultStatusText(304) == "Not Modified" &&
            HttpResponse::defaultStatusText(599) == "Server Error" && HttpResponse::defaultStatusText(-1).empty() &&
            HttpResponse::defaultStatusText(600).empty(), "HTTP media and extension status reasons");
        LegacyInterface statusHandler;
        HttpRequest statusRequest;
        statusRequest.m_method = "GET";
        statusRequest.m_queryString = "status";
        const HttpResponse custom = statusHandler.handle(statusRequest);
        check(custom.m_statusCode == 202 && custom.m_statusText == "Queued" &&
            custom.m_headers.count("X-Mode") && custom.m_headers.at("X-Mode") == "async" &&
            custom.m_body == "LEGACY_GET:status", "HttpInterface callback sets status header and body");
        check(textResponse.m_headers.size() == 1 && textResponse.m_headers.begin()->second == "second", "setting a header twice replaces its case variant");
        textResponse.setHeader("", "bad");
        textResponse.setHeader("X-Bad Field", "ignored");
        textResponse.setHeader("X-Control", std::string(1, '\n'));
        check(!textResponse.m_headers.count("X-Bad Field") && !textResponse.m_headers.count("X-Control"),
            "HttpResponse validates optional header names and values");
        check(textResponse.m_headers.find("") == textResponse.m_headers.end(), "HttpResponse ignore empty header name");
        check(HttpResponse::defaultStatusText(kHttpStatusNotFound) == "Not Found", "HttpResponse status text");
        HttpServiceOptions defaultOptions;
        check(defaultOptions.m_port == 0 && defaultOptions.m_workerThreads == 16 &&
            defaultOptions.m_requestTimeoutMs == 30000 && defaultOptions.m_maxRequestBodyBytes == 1024 * 1024 &&
            defaultOptions.m_listenHost == "0.0.0.0" && !defaultOptions.m_enableKeepAlive &&
            defaultOptions.m_enableCors && defaultOptions.m_corsAllowOrigin == "*", "HttpServiceOptions default");
        HandlerContext emptyContext;
        check(emptyContext(nullptr) == 0, "HandlerContext empty callback");

        HttpService invalidService;
        check(!invalidService.addHandler("", std::shared_ptr<HttpHandler>()), "HttpService add invalid shared handler");
        check(!invalidService.addHandler("/raw_null", static_cast<HttpHandler*>(nullptr)), "HttpService add invalid raw handler");
        HttpServiceOptions invalidOptions;
        check(!invalidService.start() && !invalidService.setOptions(invalidOptions) && !invalidService.start(invalidOptions), "HttpService start invalid port");

        TestHandler rawHandler;
        check(!invalidService.addHandler(std::string("/named\0tail", 11), &rawHandler), "route must preserve complete name");
        check(invalidService.addHandler("/fixed", &rawHandler) && invalidService.lastError().empty(),
            "successful route registration clears previous error");
        HttpService rawService;
        check(rawService.addHandler("/raw", &rawHandler), "HttpService raw handler compatibility");
        rawService.clearHandlers();
    }

    std::shared_ptr<TestHandler> handler(new TestHandler);
    std::weak_ptr<TestHandler> handlerWatcher = handler;
    std::shared_ptr<ThrowHandler> throwHandler(new ThrowHandler);
    std::shared_ptr<ReplaceHandler> replaceHandlerA(new ReplaceHandler("replace_a"));
    std::shared_ptr<ReplaceHandler> replaceHandlerB(new ReplaceHandler("replace_b"));
    std::shared_ptr<LargeHandler> largeHandler(new LargeHandler);
    std::shared_ptr<LegacyInterface> legacyInterface(new LegacyInterface);
    HttpService service;
    check(service.handleContextConnection(nullptr, handler) == 0, "HttpService null context connection");
    check(service.addHandler("/test", handler), "HttpService add shared handler");
    handler.reset();
    check(!handlerWatcher.expired(), "HttpService shared handler retained before start");
    check(service.addHandler("/throw", throwHandler), "HttpService add throw handler");
    check(service.addHandler("/replace", replaceHandlerA), "HttpService add replace handler");
    check(service.addHandler("/large", largeHandler), "HttpService add large handler");
    check(service.addHandler("/legacy", legacyInterface), "HttpService add legacy interface");
    HttpServiceOptions options;
    options.m_listenHost = "127.0.0.1";
    options.m_workerThreads = 0;
    options.m_enableKeepAlive = false;
    options.m_enableCors = true;
    options.m_requestTimeoutMs = 0;
    options.m_maxRequestBodyBytes = 64;
    options.m_corsAllowOrigin = "https://media.example.test";
    options.m_port = static_cast<uint16_t>(firstPort);
    HttpServiceOptions invalidMetadata = options;
    invalidMetadata.m_corsAllowOrigin = std::string(1, '\r');
    check(!service.setOptions(invalidMetadata), "HttpService rejects invalid configured CORS field");
    bool started = false;
    for (unsigned offset = 0; offset < 20 && firstPort + offset <= 65535; ++offset)
    {
        options.m_port = static_cast<uint16_t>(firstPort + offset);
        if (service.start(options))
        {
            started = true;
            break;
        }
    }
    check(started && service.isRunning(), "HttpService start");
    check(started && corsOrigin(options.m_port, "/test") == options.m_corsAllowOrigin,
        "HttpService configured CORS origin on wire");
    check(!service.setOptions(options), "HttpService refuses replacement configuration while running");
    if (!started)
    {
        std::cout << "HttpService test FAIL, total=" << totalCount << ", failed=" << failCount << std::endl;
        return 1;
    }

    std::function<bool(const std::string&, int32_t&, std::string&)> httpRequest =
        [&options, &httpRequestByPort](const std::string& requestText, int32_t& statusCode, std::string& body) -> bool
    {
        return httpRequestByPort(options.m_port, requestText, statusCode, body);
    };

    int32_t statusCode = 0;
    std::string body;
    if (argc == 2 && std::string(argv[1]) == "--benchmark")
    {
        RunHttpServiceSocketBenchmark(options.m_port);
        LibraryBenchmark::run("HttpService.GET.loopback.close", [&](uint32_t count) -> uint64_t {
            uint64_t bytes = 0;
            for (uint32_t i = 0; i < count; ++i)
            {
                if (!httpRequest("GET /test?name=bench HTTP/1.0\r\nHost: 127.0.0.1\r\n\r\n", statusCode, body) || statusCode != 200 || body != "GET:bench")
                {
                    throw std::runtime_error("HttpService benchmark response mismatch");
                }
                bytes += body.size();
            }
            return bytes;
        });
    }
    if (functional)
    {
        check(httpRequest("GET /test?name=cook%20test HTTP/1.0\r\nHost: 127.0.0.1\r\n\r\n", statusCode, body) &&
            statusCode == kHttpStatusOk && body == "GET:cook test", "HttpService GET");
        check(httpRequest("POST /test HTTP/1.0\r\nHost: 127.0.0.1\r\nContent-Type: text/plain\r\nContent-Length: 11\r\n\r\nhello world", statusCode, body) &&
            statusCode == kHttpStatusOk && body.find("hello world") != std::string::npos, "HttpService POST");
        const std::string chunkHeader = "POST /test HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\nTransfer-Encoding: chunked\r\n\r\n";
        check(httpRequest(chunkHeader + "2\r\nab\r\n3\r\ncde\r\n0\r\n\r\n", statusCode, body) &&
            statusCode == kHttpStatusOk && body == "{\"method\":\"POST\",\"body\":\"abcde\"}", "HttpService decoded chunked POST");
        check(httpRequest(chunkHeader + "20\r\n" + std::string(32, 'x') + "\r\n21\r\n" + std::string(33, 'y') +
            "\r\n0\r\n\r\n", statusCode, body) && statusCode == kHttpStatusPayloadTooLarge, "HttpService cumulative chunked limit");
        std::string exactBody(64, 'y');
        std::ostringstream exactRequest;
        exactRequest << "POST /test HTTP/1.0\r\nHost: 127.0.0.1\r\nContent-Length: " << exactBody.size() << "\r\n\r\n" << exactBody;
        check(httpRequest(exactRequest.str(), statusCode, body) &&
            statusCode == kHttpStatusOk && body.find(exactBody) != std::string::npos, "HttpService payload exact limit");
        check(httpRequest("GET /test?name=a%ZZ+b%2F HTTP/1.0\r\nHost: 127.0.0.1\r\n\r\n", statusCode, body) &&
            statusCode == kHttpStatusOk && body == "GET:a%ZZ b/", "HttpService GET query decode edge");
        check(httpRequest("GET /test?name=edge% HTTP/1.0\r\nHost: 127.0.0.1\r\n\r\n", statusCode, body) &&
            statusCode == kHttpStatusOk && body == "GET:edge%", "HttpService GET query trailing percent");
        check(httpRequest("DELETE /test HTTP/1.0\r\nHost: 127.0.0.1\r\n\r\n", statusCode, body) &&
            statusCode == kHttpStatusMethodNotAllowed, "HttpService method not allowed");
        std::string largeBody(128, 'x');
        std::ostringstream largeRequest;
        largeRequest << "POST /test HTTP/1.0\r\nHost: 127.0.0.1\r\nContent-Length: " << largeBody.size() << "\r\n\r\n" << largeBody;
        check(httpRequest(largeRequest.str(), statusCode, body) && statusCode == kHttpStatusPayloadTooLarge, "HttpService payload too large");
        check(httpRequest("OPTIONS /test HTTP/1.0\r\nHost: 127.0.0.1\r\n\r\n", statusCode, body) &&
            statusCode == kHttpStatusNoContent, "HttpService OPTIONS");
        check(CheckHttpServiceBodyless(options.m_port, "HEAD /large HTTP/1.0\r\nHost: localhost\r\n\r\n", 200, 200000),
            "HEAD carries representation length and zero payload bytes");
        check(CheckMediaCors(options.m_port, "/test", true, options.m_corsAllowOrigin.c_str()), "HttpService allows cross-origin conditional media headers");
        check(CheckMediaCors(options.m_port, "/test", false, options.m_corsAllowOrigin.c_str()), "HttpService exposes cross-origin media validators");
        check(CheckHttpServiceBodyless(options.m_port, "OPTIONS /test HTTP/1.0\r\nHost: localhost\r\n\r\n", 204, -1),
            "204 omits Content-Length and payload");
        check(CheckHttpServiceBodyless(options.m_port, "GET /test?unchanged HTTP/1.0\r\nHost: localhost\r\n\r\n", 304, -1),
            "304 ends after its headers");
        check(CheckHttpServiceBodyless(options.m_port, "GET /test?reset HTTP/1.0\r\nHost: localhost\r\n\r\n", 205, 0),
            "205 explicitly carries zero content length and no payload");
        check(httpRequest("GET /missing HTTP/1.0\r\nHost: 127.0.0.1\r\n\r\n", statusCode, body) &&
            statusCode == kHttpStatusNotFound, "HttpService not found");
        check(httpRequest("GET /throw HTTP/1.0\r\nHost: 127.0.0.1\r\n\r\n", statusCode, body) &&
            statusCode == kHttpStatusInternalServerError, "HttpService handler exception");
        check(httpRequest("GET /large HTTP/1.0\r\nHost: 127.0.0.1\r\n\r\n", statusCode, body) &&
            statusCode == kHttpStatusOk && body.size() == 200000, "HttpService large response");
        check(httpRequest("GET /replace HTTP/1.0\r\nHost: 127.0.0.1\r\n\r\n", statusCode, body) &&
            statusCode == kHttpStatusOk && body == "replace_a", "HttpService replace route before update");
        std::weak_ptr<ReplaceHandler> previous = replaceHandlerA;
        replaceHandlerA.reset();
        check(service.addHandler("/replace", replaceHandlerB), "HttpService replace route when running");
        check(previous.expired(), "HttpService releases replaced handler without stopping");
        check(httpRequest("GET /replace HTTP/1.0\r\nHost: 127.0.0.1\r\n\r\n", statusCode, body) &&
            statusCode == kHttpStatusOk && body == "replace_b", "HttpService replace route after update");
        check(httpRequest("GET /legacy?x=1 HTTP/1.0\r\nHost: 127.0.0.1\r\n\r\n", statusCode, body) &&
            statusCode == kHttpStatusOk && body == "LEGACY_GET:x=1", "HttpInterface legacy GET");
        check(httpRequest("POST /legacy HTTP/1.0\r\nHost: 127.0.0.1\r\nContent-Length: 3\r\n\r\nabc", statusCode, body) &&
            statusCode == kHttpStatusOk && body == "LEGACY_POST:abc", "HttpInterface legacy POST");
    }

    if (pressure)
    {
        std::atomic<int32_t> threadFailCount(0);
        std::vector<std::thread> threads;
        for (int32_t threadIndex = 0; threadIndex < 8; ++threadIndex)
        {
            threads.push_back(std::thread([threadIndex, &httpRequest, &threadFailCount]() -> void
            {
                for (int32_t loopIndex = 0; loopIndex < 30; ++loopIndex)
                {
                    std::ostringstream requestStream;
                    requestStream << "GET /test?name=t" << threadIndex << "_" << loopIndex << " HTTP/1.0\r\nHost: 127.0.0.1\r\n\r\n";
                    int32_t status = 0;
                    std::string responseBody;
                    if (!httpRequest(requestStream.str(), status, responseBody) ||
                        status != kHttpStatusOk ||
                        responseBody.find("GET:t") != 0)
                    {
                        threadFailCount.fetch_add(1);
                    }
                }
            }));
        }
        for (size_t i = 0; i < threads.size(); ++i)
        {
            threads[i].join();
        }
        check(threadFailCount.load() == 0, "HttpService multithread pressure");
        std::shared_ptr<TestHandler> retainedHandler = handlerWatcher.lock();
        check(retainedHandler.get() != nullptr && retainedHandler->m_count.load() >= (functional ? 246 : 240), "HttpService handler count");
        retainedHandler.reset();

        std::atomic<int32_t> processFailCount(0);
        std::vector<std::thread> processThreads;
        const std::string exePath = CSystem::currentExecutablePathUtf8();
        for (int32_t processIndex = 0; processIndex < 4; ++processIndex)
        {
            processThreads.push_back(std::thread([processIndex, &processFailCount, &exePath, &options]() -> void
            {
                uint16_t processPort = static_cast<uint16_t>(1024 + (options.m_port + 100 + 32 * processIndex) % 64000);
                if (!CheckChildService(exePath, processPort))
                {
                    processFailCount.fetch_add(1);
                }
            }));
        }
        for (size_t i = 0; i < processThreads.size(); ++i)
        {
            processThreads[i].join();
        }
        check(processFailCount.load() == 0, "HttpService multiprocess pressure");
    }

    if (functional)
    {
        std::shared_ptr<LegacyInterface> runInterface(new LegacyInterface);
        std::weak_ptr<LegacyInterface> runInterfaceWatcher = runInterface;
        std::vector<std::pair<std::string, std::shared_ptr<HttpInterface>>> runInterfaces;
        runInterfaces.push_back(std::make_pair(std::string("/run"), runInterface));
        runInterface.reset();
        // 派生业务服务可以将受保护的借用路由表交回 run 进行重启。
        struct RouteTableService : HttpService
        {
            using HttpService::m_vecInterface;
        };
        RouteTableService runService;
        // 兼容入口先登记路由；端口0在绑定前返回。随后用显式回环地址启动同一服务。
        runService.run(0, runInterfaces);
        check(!runService.isRunning(), "zero port did not bind");
        HttpServiceOptions loopbackOptions = options;
        loopbackOptions.m_listenHost = "127.0.0.1";
        loopbackOptions.m_maxRequestBodyBytes = -1;
        loopbackOptions.m_enableCors = false;
        bool runStarted = false;
        for (int32_t offset = 40; offset < 60; ++offset)
        {
            loopbackOptions.m_port = static_cast<uint16_t>(options.m_port + offset);
            if (!runService.setOptions(loopbackOptions)) { continue; }
            HttpServiceOptions invalid = loopbackOptions;
            invalid.m_maxRequestBodyBytes = -2;
            if (!runService.setOptions(invalid) && runService.start())
            {
                runStarted = true;
                break;
            }
        }
        check(runStarted, "HttpService shared interface run");
        check(runStarted && corsOrigin(loopbackOptions.m_port, "/run") == "missing", "HttpService disabled CORS on wire");
        check(runStarted && CheckMediaCors(loopbackOptions.m_port, "/run", true, nullptr), "HttpService disables preflight CORS fields");
        const std::string aboveDefault(1024 * 1024 + 1, 'a');
        const std::string unlimited = "POST /run HTTP/1.0\r\nHost: 127.0.0.1\r\nContent-Length: " +
            std::to_string(aboveDefault.size()) + "\r\n\r\n" + aboveDefault;
        check(runStarted && httpRequestByPort(loopbackOptions.m_port, unlimited, statusCode, body) &&
            statusCode == kHttpStatusOk && body == "LEGACY_POST:" + aboveDefault, "HttpService unlimited body over default boundary");
        runService.run(loopbackOptions.m_port, runInterfaces);
        check(runService.isRunning() && corsOrigin(loopbackOptions.m_port, "/run") == "missing",
            "HttpService shared run preserves configured CORS");
        std::vector<std::pair<std::string, HttpInterface*>> borrowedRoutes;
        std::vector<std::pair<std::string, std::shared_ptr<HttpInterface>>> invalidRoutes;
        invalidRoutes.emplace_back("/first", std::make_shared<LegacyInterface>());
        std::weak_ptr<HttpInterface> partial = invalidRoutes.front().second;
        invalidRoutes.emplace_back("/second", std::shared_ptr<HttpInterface>());
        runService.run(loopbackOptions.m_port, invalidRoutes);
        invalidRoutes.clear();
        check(!runService.isRunning() && !runService.lastError().empty() && partial.expired(),
            "shared route batch failure releases partial registration and preserves error");
        borrowedRoutes.emplace_back("/first", runInterfaces.front().second.get());
        borrowedRoutes.emplace_back("/second", nullptr);
        runService.run(loopbackOptions.m_port, borrowedRoutes);
        check(!runService.isRunning() && !runService.lastError().empty(),
            "borrowed route batch failure keeps service stopped");
        borrowedRoutes.clear();
        borrowedRoutes.push_back(std::make_pair(std::string("/run"), runInterfaces[0].second.get()));
        runService.run(loopbackOptions.m_port, borrowedRoutes);
        check(runService.isRunning() && corsOrigin(loopbackOptions.m_port, "/run") == "missing" &&
            httpRequestByPort(loopbackOptions.m_port, unlimited, statusCode, body) && statusCode == kHttpStatusOk,
            "HttpService borrowed run preserves configured CORS and body limit");
        runService.run(loopbackOptions.m_port, runService.m_vecInterface);
        check(runService.isRunning() && runService.m_vecInterface.size() == 1 &&
            httpRequestByPort(loopbackOptions.m_port,
                "POST /run HTTP/1.0\r\nHost: 127.0.0.1\r\nContent-Length: 1\r\n\r\nx", statusCode, body) &&
            statusCode == kHttpStatusOk && body == "LEGACY_POST:x", "borrowed route table survives restart by reference");
        runService.run(loopbackOptions.m_port, runInterfaces);
        check(!runInterfaceWatcher.expired(), "HttpService shared interface retained after caller reset");
        runInterfaces.clear();
        check(!runInterfaceWatcher.expired(), "HttpService shared interface retained by route");
        runService.clearHandlers();
        check(CheckHttpServiceOptions(loopbackOptions.m_port),
            "HttpService single worker, keep-alive and configured request deadline");
        check(runInterfaceWatcher.expired(), "HttpService shared interface released after clearHandlers");
    }

    service.stop();
    check(!service.isRunning(), "HttpService stop");
    check(!handlerWatcher.expired(), "HttpService shared handler retained after stop");
    service.clearHandlers();
    check(handlerWatcher.expired(), "HttpService shared handler released after clearHandlers");
    std::cout << "HttpService test " << (failCount == 0 ? "PASS" : "FAIL") <<
        ", total=" << totalCount << ", failed=" << failCount << std::endl;
    return failCount == 0 ? 0 : 1;
}

int main(int argc, char* argv[])
{
    const std::string mode = argc > 1 ? argv[1] : "";
    if (mode == "--client" || mode == "--reject" || mode == "--public" || mode == "--reuse" ||
        mode == "--uncached-ca" || mode == "--large-ca-default" || mode == "--large-ca-unlimited" ||
        mode == "--limits" || mode == "--capacity-default" || mode == "--capacity-unlimited" ||
        (mode == "--benchmark" && argc >= 3))
    {
        return HttpClientCasesMain(argc, argv);
    }
    if (mode == "--httpservice-child")
    {
        return argc == 3 ? RunServiceCases(argc, argv, false, false) : 2;
    }
    if (argc == 1 || (argc == 2 && (mode == "--all" || mode == "--benchmark")))
    {
        return RunServiceCases(argc, argv, true, true);
    }
    const LibraryTestCase cases[] = {
        {kServiceFunctional, false, []() -> int { return RunServiceCases(0, nullptr, true, false); }},
        {kServicePressure, true, []() -> int { return RunServiceCases(0, nullptr, false, true); }}
    };
    return LibraryCaseSelection::run(argc, argv, cases);
}
#endif