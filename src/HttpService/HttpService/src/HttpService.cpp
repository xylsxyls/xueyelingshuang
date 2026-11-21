#include "HttpService.h"
#include "HandlerContext.h"
#include "HttpHandler.h"
#include "HttpInterface.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "HttpServiceHelper.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "civetweb.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <map>
#include <memory>
#include <new>

// 自动 OPTIONS 响应和 CORS 预检公布相同的请求方法。
static const char kAllowedMethods[] = "GET, HEAD, POST, PUT, DELETE, PATCH, OPTIONS";

HttpService::HttpService() :
m_context(nullptr),
m_running(false)
{

}

HttpService::~HttpService()
{
    stop();
}

bool HttpService::addHandler(const std::string& uri, const std::shared_ptr<HttpHandler>& handler)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    if (uri.empty() || uri.find('\0') != std::string::npos || handler.get() == nullptr)
    {
        m_lastError = "invalid http handler";
        return false;
    }

    for (size_t i = 0; i < m_routes.size(); ++i)
    {
        if (m_routes[i].first == uri)
        {
            if (m_running.load())
            {
                // 运行期每条路由只有一个回调上下文。正在处理的请求持有自己的快照。
                m_contexts[i]->setHandler(handler);
            }
            m_routes[i].second = handler;
            m_lastError.clear();
            return true;
        }
    }

    try
    {
        m_routes.push_back(std::make_pair(uri, handler));
    }
    catch (...)
    {
        m_lastError = "save http handler failed";
        return false;
    }
    if (m_running.load())
    {
        if (!registerRoute(uri, handler))
        {
            m_routes.pop_back();
            return false;
        }
    }
    m_lastError.clear();
    return true;
}

bool HttpService::addHandler(const std::string& uri, HttpHandler* handler)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    if (handler == nullptr)
    {
        m_lastError = "invalid http handler";
        return false;
    }

    // compatibleDeleter入参：currentHandler是旧接口传入的裸指针。
    // compatibleDeleter出参：无。
    // compatibleDeleter返回值：无，兼容入口不接管对象释放。
    auto compatibleDeleter = [](HttpHandler* currentHandler) -> void
    {
        (void)currentHandler;
    };
    std::shared_ptr<HttpHandler> compatibleHandler(handler, compatibleDeleter);
    return addHandler(uri, compatibleHandler);
}

void HttpService::clearHandlers()
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    m_vecInterface.clear();
    stop();
    m_routes.clear();
    m_lastError.clear();
}

bool HttpService::start(const HttpServiceOptions& options)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    stop();
    return setOptions(options) && start();
}

bool HttpService::setOptions(const HttpServiceOptions& options)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (m_running.load())
    {
        m_lastError = "http service is already running";
        return false;
    }
    if (options.m_port == 0)
    {
        m_lastError = "http port is zero";
        return false;
    }
    if (options.m_maxRequestBodyBytes < -1)
    {
        m_lastError = "invalid http body limit";
        return false;
    }
    if (!HttpServiceHelper::validValue(options.m_listenHost) ||
        (options.m_enableCors && !HttpServiceHelper::validValue(options.m_corsAllowOrigin)))
    {
        m_lastError = "invalid http listen address or CORS origin";
        return false;
    }
    m_options = options;
    if (m_options.m_workerThreads <= 0)
    {
        m_options.m_workerThreads = 16;
    }
    if (m_options.m_requestTimeoutMs <= 0)
    {
        m_options.m_requestTimeoutMs = 30000;
    }
    if (m_options.m_maxRequestBodyBytes == 0)
    {
        m_options.m_maxRequestBodyBytes = 1024 * 1024;
    }

    m_lastError.clear();
    return true;
}

bool HttpService::start()
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (m_running.load())
    {
        m_lastError.clear();
        return true;
    }
    if (m_options.m_port == 0)
    {
        m_lastError = "http port is zero";
        return false;
    }
    // buildListenPort入参：listenOptions是当前HTTP服务启动配置。
    // buildListenPort出参：无。
    // buildListenPort返回值：返回civetweb要求的监听端口文本，可能是端口，也可能是IP:端口。
    auto buildListenPort = [](const HttpServiceOptions& listenOptions) -> std::string
    {
        std::string port = CStringManager::toStringInt32(listenOptions.m_port);
        if (listenOptions.m_listenHost.empty() || listenOptions.m_listenHost == "0.0.0.0")
        {
            return port;
        }
        return listenOptions.m_listenHost + ":" + port;
    };

    std::vector<std::string> optionText;
    optionText.push_back("listening_ports");
    optionText.push_back(buildListenPort(m_options));
    optionText.push_back("num_threads");
    optionText.push_back(CStringManager::toStringInt32(m_options.m_workerThreads));
    optionText.push_back("request_timeout_ms");
    optionText.push_back(CStringManager::toStringInt32(m_options.m_requestTimeoutMs));
    optionText.push_back("enable_keep_alive");
    optionText.push_back(m_options.m_enableKeepAlive ? "yes" : "no");

    // 预检由路由按本服务的 CORS 配置应答，避免 CivetWeb 使用其默认来源提前响应。
    optionText.push_back("access_control_allow_methods");
    optionText.push_back("");
    optionText.push_back("access_control_allow_origin");
    optionText.push_back("");

    std::vector<const char*> rawOptions;
    for (size_t i = 0; i < optionText.size(); ++i)
    {
        rawOptions.push_back(optionText[i].c_str());
    }
    rawOptions.push_back(nullptr);

    struct mg_callbacks callbacks;
    std::memset(&callbacks, 0, sizeof(callbacks));
    m_context = mg_start(&callbacks, nullptr, &rawOptions[0]);
    if (m_context == nullptr)
    {
        m_lastError = "mg_start failed";
        return false;
    }

    m_running.store(true);
    for (size_t i = 0; i < m_routes.size(); ++i)
    {
        if (!registerRoute(m_routes[i].first, m_routes[i].second))
        {
            stop();
            return false;
        }
    }
    m_lastError.clear();
    return true;
}

void HttpService::stop()
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    if (m_context != nullptr)
    {
        mg_stop(m_context);
        m_context = nullptr;
    }
    m_contexts.clear();
    m_running.store(false);
}

bool HttpService::isRunning() const
{
    return m_running.load();
}

std::string HttpService::lastError()
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    return m_lastError;
}

void HttpService::run(uint16_t port, const std::vector<std::pair<std::string, HttpInterface*>>& vecInterface)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    // 派生服务可能传入自身的受保护路由表，停止旧服务前先取得本次注册列表。
    std::vector<std::pair<std::string, HttpInterface*>> routes(vecInterface);
    clearHandlers();
    m_vecInterface.swap(routes);
    for (size_t i = 0; i < m_vecInterface.size(); ++i)
    {
        if (!addHandler(m_vecInterface[i].first, m_vecInterface[i].second))
        {
            m_vecInterface.clear();
            m_routes.clear();
            return;
        }
    }

    HttpServiceOptions options = m_options;
    options.m_port = port;
    start(options);
}

void HttpService::run(uint16_t port, const std::vector<std::pair<std::string, std::shared_ptr<HttpInterface>>>& vecInterface)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    clearHandlers();
    for (size_t i = 0; i < vecInterface.size(); ++i)
    {
        if (!addHandler(vecInterface[i].first, std::static_pointer_cast<HttpHandler>(vecInterface[i].second)))
        {
            m_routes.clear();
            return;
        }
    }

    HttpServiceOptions options = m_options;
    options.m_port = port;
    start(options);
}

void HttpService::loop()
{
    while (isRunning())
    {
        CSystem::Sleep(1000);
    }
}

int32_t HttpService::handleContextConnection(void* conn, const std::shared_ptr<HttpHandler>& handler)
{
    return handleConnection(conn, handler);
}

bool HttpService::registerRoute(const std::string& uri, const std::shared_ptr<HttpHandler>& handler)
{
    if (m_context == nullptr || handler.get() == nullptr)
    {
        m_lastError = "invalid route context";
        return false;
    }
    std::shared_ptr<HandlerContext> handlerContext(new (std::nothrow) HandlerContext(this, handler));
    if (handlerContext.get() == nullptr)
    {
        m_lastError = "new handler context failed";
        return false;
    }
    try
    {
        m_contexts.push_back(handlerContext);
    }
    catch (...)
    {
        m_lastError = "save handler context failed";
        return false;
    }

    // requestCallback入参：conn是civetweb传入的底层HTTP连接，cbdata是注册路由时传入的HandlerContext。
    // requestCallback出参：无。
    // requestCallback返回值：返回1表示请求已处理，返回0表示当前回调无法处理。
    auto requestCallback = [](struct mg_connection* conn, void* cbdata) -> int32_t
    {
        HandlerContext* currentContext = static_cast<HandlerContext*>(cbdata);
        if (currentContext == nullptr)
        {
            return 0;
        }
        return (*currentContext)(conn);
    };

    mg_set_request_handler(m_context, uri.c_str(), requestCallback, handlerContext.get());
    return true;
}

int32_t HttpService::handleConnection(void* conn, const std::shared_ptr<HttpHandler>& handler)
{
    if (conn == nullptr || handler.get() == nullptr)
    {
        return 0;
    }

    HttpRequest request;
    HttpResponse errorResponse;
    if (!buildRequest(conn, request, errorResponse))
    {
        writeResponse(conn, errorResponse);
        return 1;
    }

    if (request.m_method == "OPTIONS")
    {
        HttpResponse response = HttpResponse::text("", kHttpStatusNoContent);
        response.setHeader("Allow", kAllowedMethods);
        writeResponse(conn, response);
        return 1;
    }

    HttpResponse response;
    try
    {
        response = handler->handle(request);
    }
    catch (...)
    {
        response = HttpResponse::text("Internal Server Error", kHttpStatusInternalServerError);
    }
    writeResponse(conn, response);
    return 1;
}

bool HttpService::buildRequest(void* connData, HttpRequest& request, HttpResponse& errorResponse)
{
    // hexValue入参：c是URL编码中的单个十六进制字符。
    // hexValue出参：无。
    // hexValue返回值：返回0到15表示合法十六进制值，返回-1表示非法字符。
    auto hexValue = [](char c) -> int32_t
    {
        if (c >= '0' && c <= '9')
        {
            return c - '0';
        }
        if (c >= 'a' && c <= 'f')
        {
            return c - 'a' + 10;
        }
        if (c >= 'A' && c <= 'F')
        {
            return c - 'A' + 10;
        }
        return -1;
    };

    // urlDecode入参：text是需要解码的URL片段。
    // urlDecode出参：无。
    // urlDecode返回值：返回把+和%XX编码还原后的文本。
    auto urlDecode = [&hexValue](const std::string& text) -> std::string
    {
        std::string result;
        result.reserve(text.size());
        for (size_t i = 0; i < text.size(); ++i)
        {
            if (text[i] == '+')
            {
                result.push_back(' ');
            }
            else if (text[i] == '%' && i + 2 < text.size())
            {
                int32_t hi = hexValue(text[i + 1]);
                int32_t lo = hexValue(text[i + 2]);
                if (hi >= 0 && lo >= 0)
                {
                    result.push_back(static_cast<char>((hi << 4) | lo));
                    i += 2;
                }
                else
                {
                    result.push_back(text[i]);
                }
            }
            else
            {
                result.push_back(text[i]);
            }
        }
        return result;
    };

    // parseQuery入参：queryString是URL中的查询字符串。
    // parseQuery出参：query会被填充为解析后的键值对。
    // parseQuery返回值：无。
    auto parseQuery = [&urlDecode](const std::string& queryString, std::map<std::string, std::string>& query) -> void
    {
        size_t start = 0;
        while (start <= queryString.size())
        {
            size_t end = queryString.find('&', start);
            if (end == std::string::npos)
            {
                end = queryString.size();
            }
            std::string part = queryString.substr(start, end - start);
            if (!part.empty())
            {
                size_t eq = part.find('=');
                if (eq == std::string::npos)
                {
                    query[urlDecode(part)] = "";
                }
                else
                {
                    query[urlDecode(part.substr(0, eq))] = urlDecode(part.substr(eq + 1));
                }
            }
            if (end == queryString.size())
            {
                break;
            }
            start = end + 1;
        }
    };

    if (connData == nullptr)
    {
        errorResponse = HttpResponse::text("Bad Request", kHttpStatusBadRequest);
        return false;
    }

    struct mg_connection* conn = static_cast<struct mg_connection*>(connData);
    const struct mg_request_info* requestInfo = mg_get_request_info(conn);
    if (requestInfo == nullptr)
    {
        errorResponse = HttpResponse::text("Bad Request", kHttpStatusBadRequest);
        return false;
    }

    int64_t contentLength = requestInfo->content_length < 0 ? 0 : requestInfo->content_length;
    request.m_method = requestInfo->request_method == nullptr ? "" : requestInfo->request_method;
    request.m_uri = requestInfo->local_uri == nullptr ?
        (requestInfo->request_uri == nullptr ? "" : requestInfo->request_uri) : requestInfo->local_uri;
    request.m_queryString = requestInfo->query_string == nullptr ? "" : requestInfo->query_string;
    request.m_remoteIp = requestInfo->remote_addr == nullptr ? "" : requestInfo->remote_addr;
    request.m_remotePort = requestInfo->remote_port;
    request.m_contentLength = contentLength;
    request.m_isSsl = requestInfo->is_ssl != 0;
    parseQuery(request.m_queryString, request.m_query);

    for (int32_t i = 0; i < requestInfo->num_headers; ++i)
    {
        const char* name = requestInfo->http_headers[i].name;
        const char* value = requestInfo->http_headers[i].value;
        if (name != nullptr && value != nullptr)
        {
            request.m_headers[name] = value;
        }
    }

    int64_t maxStringSize = (std::numeric_limits<int64_t>::max)();
    if (sizeof(size_t) < sizeof(int64_t))
    {
        maxStringSize = static_cast<int64_t>((std::numeric_limits<size_t>::max)());
    }
    if ((m_options.m_maxRequestBodyBytes >= 0 && contentLength > m_options.m_maxRequestBodyBytes) || contentLength > maxStringSize)
    {
        errorResponse = HttpResponse::text("Payload Too Large", kHttpStatusPayloadTooLarge);
        return false;
    }

    const char* transferEncoding = mg_get_header(conn, "Transfer-Encoding");
    if (requestInfo->content_length < 0 && transferEncoding != nullptr &&
        HttpServiceHelper::sameName(transferEncoding, "chunked"))
    {
        // CivetWeb 负责解析分块。按实际收到的字节检查同一上限，不预分配未声明的正文。
        char buffer[8192];
        const uint64_t limit = m_options.m_maxRequestBodyBytes < 0 ?
            static_cast<uint64_t>(maxStringSize) : static_cast<uint64_t>(m_options.m_maxRequestBodyBytes);
        int bytes;
        while ((bytes = mg_read(conn, buffer, sizeof(buffer))) > 0)
        {
            if (static_cast<uint64_t>(bytes) > limit - request.m_body.size())
            {
                errorResponse = HttpResponse::text("Payload Too Large", kHttpStatusPayloadTooLarge);
                return false;
            }
            try
            {
                request.m_body.append(buffer, static_cast<size_t>(bytes));
            }
            catch (...)
            {
                errorResponse = HttpResponse::text("Payload Too Large", kHttpStatusPayloadTooLarge);
                return false;
            }
        }
        if (bytes < 0)
        {
            errorResponse = HttpResponse::text("Bad Request", kHttpStatusBadRequest);
            return false;
        }
        request.m_contentLength = static_cast<int64_t>(request.m_body.size());
    }
    else if (contentLength > 0)
    {
        int64_t totalRead = 0;
        try
        {
            request.m_body.resize(static_cast<size_t>(contentLength));
        }
        catch (...)
        {
            errorResponse = HttpResponse::text("Payload Too Large", kHttpStatusPayloadTooLarge);
            return false;
        }

        const int32_t readChunkBytes = 64 * 1024;
        while (totalRead < contentLength)
        {
            int64_t unreadBytes = contentLength - totalRead;
            size_t currentReadBytes = unreadBytes > readChunkBytes ?
                static_cast<size_t>(readChunkBytes) : static_cast<size_t>(unreadBytes);
            int32_t readSize = mg_read(conn,
                &request.m_body[static_cast<size_t>(totalRead)],
                currentReadBytes);
            if (readSize <= 0)
            {
                break;
            }
            totalRead += readSize;
        }
        if (totalRead != contentLength)
        {
            request.m_body.resize(static_cast<size_t>(totalRead));
            errorResponse = HttpResponse::text("Bad Request", kHttpStatusBadRequest);
            return false;
        }
    }
    return true;
}

void HttpService::appendCorsHeaders(std::string& headers, const HttpResponse& response) const
{
    if (!m_options.m_enableCors)
    {
        return;
    }
    if (!HttpServiceHelper::hasHeader(response.m_headers, "Access-Control-Allow-Origin"))
    {
        headers.append("Access-Control-Allow-Origin: ").append(m_options.m_corsAllowOrigin.c_str()).append("\r\n");
    }
    if (!HttpServiceHelper::hasHeader(response.m_headers, "Access-Control-Allow-Headers"))
    {
        headers.append("Access-Control-Allow-Headers: Content-Type, Authorization, Range, If-Range, If-None-Match, X-Requested-With, X-Idempotency-Key, X-Cook-Run-Id, X-Cook-Request-Id, X-Cook-Attempt, X-Cook-Trace-Id, X-Cook-Request-Begin-Ms, X-Cook-Queue-Wait-Ms, X-Cook-Purpose\r\n");
    }
    if (!HttpServiceHelper::hasHeader(response.m_headers, "Access-Control-Expose-Headers"))
    {
        headers.append("Access-Control-Expose-Headers: Content-Range, ETag\r\n");
    }
    if (!HttpServiceHelper::hasHeader(response.m_headers, "Access-Control-Allow-Methods"))
    {
        headers.append("Access-Control-Allow-Methods: ").append(kAllowedMethods).append("\r\n");
    }
}

void HttpService::writeResponse(void* connData, const HttpResponse& response)
{
    if (connData == nullptr)
    {
        return;
    }

    struct mg_connection* conn = static_cast<struct mg_connection*>(connData);
    std::string statusText = response.m_statusText.empty() || !HttpServiceHelper::validValue(response.m_statusText) ?
        HttpResponse::defaultStatusText(response.m_statusCode) : response.m_statusText;

    std::string headers;
    headers.reserve(512);
    headers.append("HTTP/1.1 ").append(std::to_string(response.m_statusCode)).append(" ").append(statusText.c_str()).append("\r\n");
    if (!HttpServiceHelper::hasHeader(response.m_headers, "Content-Type"))
    {
        const bool validType = !response.m_contentType.empty() && HttpServiceHelper::validValue(response.m_contentType);
        headers.append("Content-Type: ").append(validType ? response.m_contentType : "application/octet-stream").append("\r\n");
    }
    const mg_request_info* info = mg_get_request_info(conn);
    const bool head = info && info->request_method && std::strcmp(info->request_method, "HEAD") == 0;
    const int32_t code = response.m_statusCode;
    // HEAD 可以预告表示的长度。1xx、204 和 304 省略长度，205 显式标示零长度。
    const bool omitLength = (code >= 100 && code < 200) || code == 204 || code == 304;
    if (!omitLength)
    {
        headers.append("Content-Length: ").append(std::to_string(code == 205 ? size_t(0) : response.m_body.size())).append("\r\n");
    }
    const bool sendBody = !head && !omitLength && code != 205;
    bool keep = false;
    if (!response.m_closeConnection && m_options.m_enableKeepAlive)
    {
        const char* requested = mg_get_header(conn, "Connection");
        keep = requested ? HttpServiceHelper::sameName(requested, "keep-alive") :
            (info && info->http_version && std::strcmp(info->http_version, "1.1") == 0);
    }
    headers.append(keep ? "Connection: keep-alive\r\n" : "Connection: close\r\n");
    appendCorsHeaders(headers, response);

    std::map<std::string, std::string>::const_iterator it = response.m_headers.begin();
    for (; it != response.m_headers.end(); ++it)
    {
        const std::string& name = it->first;
        if (!HttpServiceHelper::validName(name) || !HttpServiceHelper::validValue(it->second) ||
            HttpServiceHelper::sameName(name, "Content-Length") || HttpServiceHelper::sameName(name, "Transfer-Encoding") ||
            HttpServiceHelper::sameName(name, "Connection"))
        {
            continue;
        }
        headers.append(name).append(": ").append(it->second).append("\r\n");
    }
    headers.append("\r\n");
    // 小响应的头和正文一次交给 socket；大正文始终借用原缓冲分段写，避免整段复制。
    const bool combined = response.m_body.size() <= 1024;
    if (sendBody && combined)
    {
        headers.append(response.m_body);
    }
    const std::string* segments[] = {&headers, &response.m_body};
    for (size_t index = 0; index < (sendBody && !combined ? 2u : 1u); ++index)
    {
        const std::string& segment = *segments[index];
        size_t offset = 0;
        while (offset < segment.size())
        {
            const size_t count = (std::min)(segment.size() - offset, static_cast<size_t>(64 * 1024));
            const int32_t writeSize = mg_write(conn, segment.data() + offset, count);
            if (writeSize <= 0)
            {
                return;
            }
            offset += static_cast<size_t>(writeSize);
        }
    }
}

// 下列示例已用 Windows x64 Release 单独编译运行。
// #include "HttpService/HttpServiceAPI.h"
// #include <cstdlib>
// #include <iostream>
// #include <memory>
//
// int main(int argc, char** argv)
// {
//     if (argc != 2) { return 2; }
//     HttpService service;
//     std::shared_ptr<HttpInterface> endpoint(new HttpInterface);
//     if (!service.addHandler("/api/recipes", endpoint)) { return 1; }
//     HttpServiceOptions options;
//     options.m_listenHost = "127.0.0.1";
//     options.m_port = static_cast<uint16_t>(std::atoi(argv[1]));
//     options.m_workerThreads = 4;
//     if (!service.setOptions(options) || !service.start())
//     {
//         std::cerr << service.lastError() << std::endl;
//         return 1;
//     }
//     const bool running = service.isRunning();
//     service.stop();
//     service.clearHandlers();
//     return running && !service.isRunning() ? 0 : 1;
// }