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

    if (uri.empty() || handler.get() == nullptr)
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
                if (!registerRoute(uri, handler))
                {
                    return false;
                }
            }
            m_routes[i].second = handler;
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
    m_options = options;
    if (m_options.m_port == 0)
    {
        m_lastError = "http port is zero";
        return false;
    }
    if (m_options.m_workerThreads <= 0)
    {
        m_options.m_workerThreads = 16;
    }
    if (m_options.m_requestTimeoutMs <= 0)
    {
        m_options.m_requestTimeoutMs = 30000;
    }
    if (m_options.m_maxRequestBodyBytes <= 0)
    {
        m_options.m_maxRequestBodyBytes = 1024 * 1024;
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

    clearHandlers();
    m_vecInterface = vecInterface;
    for (size_t i = 0; i < m_vecInterface.size(); ++i)
    {
        addHandler(m_vecInterface[i].first, m_vecInterface[i].second);
    }

    HttpServiceOptions options;
    options.m_port = port;
    start(options);
}

void HttpService::run(uint16_t port, const std::vector<std::pair<std::string, std::shared_ptr<HttpInterface>>>& vecInterface)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    clearHandlers();
    for (size_t i = 0; i < vecInterface.size(); ++i)
    {
        addHandler(vecInterface[i].first, std::static_pointer_cast<HttpHandler>(vecInterface[i].second));
    }

    HttpServiceOptions options;
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
        response.setHeader("Allow", "GET, POST, PUT, DELETE, PATCH, OPTIONS");
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
    if (contentLength > m_options.m_maxRequestBodyBytes || contentLength > maxStringSize)
    {
        errorResponse = HttpResponse::text("Payload Too Large", kHttpStatusPayloadTooLarge);
        return false;
    }

    if (contentLength > 0)
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

void HttpService::writeCorsHeaders(void* connData, const HttpResponse& response) const
{
    if (!m_options.m_enableCors)
    {
        return;
    }
    if (connData == nullptr)
    {
        return;
    }

    struct mg_connection* conn = static_cast<struct mg_connection*>(connData);
    if (!HttpServiceHelper::hasHeader(response.m_headers, "Access-Control-Allow-Origin"))
    {
        mg_printf(conn, "Access-Control-Allow-Origin: %s\r\n", m_options.m_corsAllowOrigin.c_str());
    }
    if (!HttpServiceHelper::hasHeader(response.m_headers, "Access-Control-Allow-Headers"))
    {
        mg_printf(conn, "Access-Control-Allow-Headers: Content-Type, Authorization, X-Requested-With\r\n");
    }
    if (!HttpServiceHelper::hasHeader(response.m_headers, "Access-Control-Allow-Methods"))
    {
        mg_printf(conn, "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, PATCH, OPTIONS\r\n");
    }
}

void HttpService::writeResponse(void* connData, const HttpResponse& response)
{
    if (connData == nullptr)
    {
        return;
    }

    struct mg_connection* conn = static_cast<struct mg_connection*>(connData);
    std::string statusText = response.m_statusText.empty() ?
        HttpResponse::defaultStatusText(response.m_statusCode) : response.m_statusText;

    mg_printf(conn, "HTTP/1.1 %d %s\r\n", response.m_statusCode, statusText.c_str());
    if (!HttpServiceHelper::hasHeader(response.m_headers, "Content-Type"))
    {
        mg_printf(conn, "Content-Type: %s\r\n", response.m_contentType.c_str());
    }
    mg_printf(conn, "Content-Length: %s\r\n",
        CStringManager::toStringInt64(static_cast<int64_t>(response.m_body.size())).c_str());
    mg_printf(conn, "Connection: %s\r\n", response.m_closeConnection ? "close" : "keep-alive");
    writeCorsHeaders(connData, response);

    std::map<std::string, std::string>::const_iterator it = response.m_headers.begin();
    for (; it != response.m_headers.end(); ++it)
    {
        mg_printf(conn, "%s: %s\r\n", it->first.c_str(), it->second.c_str());
    }
    mg_printf(conn, "\r\n");
    if (!response.m_body.empty())
    {
        size_t totalWrite = 0;
        const size_t writeChunkBytes = 64 * 1024;
        while (totalWrite < response.m_body.size())
        {
            size_t unreadBytes = response.m_body.size() - totalWrite;
            size_t currentWriteBytes = unreadBytes > writeChunkBytes ? writeChunkBytes : unreadBytes;
            int32_t writeSize = mg_write(conn, response.m_body.data() + totalWrite, currentWriteBytes);
            if (writeSize <= 0)
            {
                break;
            }
            totalWrite += static_cast<size_t>(writeSize);
        }
    }
}

//#include "HandlerContext.h"
//#include "HttpHandler.h"
//#include "HttpInterface.h"
//#include "CSystem/CSystemAPI.h"
//#include "CStringManager/CStringManagerAPI.h"
//#include "civetweb.h"
//#include <atomic>
//#include <cstdlib>
//#include <functional>
//#include <iostream>
//#include <memory>
//#include <sstream>
//#include <thread>
//
//int main(int argc, char* argv[])
//{
//	int32_t totalCount = 0;
//	int32_t failCount = 0;
//	std::function<void(bool, const std::string&)> check = [&totalCount, &failCount](bool ok, const std::string& name) -> void
//	{
//		++totalCount;
//		std::cout << (ok ? "[PASS] " : "[FAIL] ") << name << std::endl;
//		if (!ok)
//		{
//			++failCount;
//		}
//	};
//	std::function<int32_t(const char*, int32_t)> envInt = [](const char* name, int32_t defaultValue) -> int32_t
//	{
//		const char* value = std::getenv(name);
//		return value == nullptr ? defaultValue : static_cast<int32_t>(std::atoi(value));
//	};
//	std::function<bool(uint16_t, const std::string&, int32_t&, std::string&)> httpRequestByPort =
//		[](uint16_t port, const std::string& requestText, int32_t& statusCode, std::string& body) -> bool
//	{
//		char error[256] = {};
//		struct mg_connection* conn = mg_download("127.0.0.1", port, 0, error, sizeof(error), "%s", requestText.c_str());
//		if (conn == nullptr)
//		{
//			body = error;
//			statusCode = 0;
//			return false;
//		}
//		const struct mg_response_info* responseInfo = mg_get_response_info(conn);
//		statusCode = responseInfo == nullptr ? 0 : responseInfo->status_code;
//		char buffer[1024] = {};
//		int32_t readSize = 0;
//		body.clear();
//		while ((readSize = mg_read(conn, buffer, sizeof(buffer))) > 0)
//		{
//			body.append(buffer, static_cast<size_t>(readSize));
//		}
//		mg_close_connection(conn);
//		return true;
//	};
//
//	class TestHandler : public HttpHandler
//	{
//	public:
//		TestHandler() :
//		m_count(0)
//		{
//
//		}
//
//		virtual HttpResponse handle(const HttpRequest& request)
//		{
//			++m_count;
//			if (request.m_method == "GET")
//			{
//				std::map<std::string, std::string>::const_iterator it = request.m_query.find("name");
//				std::string name = it == request.m_query.end() ? "" : it->second;
//				return HttpResponse::text("GET:" + name);
//			}
//			if (request.m_method == "POST")
//			{
//				return HttpResponse::json("{\"method\":\"POST\",\"body\":\"" + request.m_body + "\"}");
//			}
//			return HttpResponse::text("method not allowed", kHttpStatusMethodNotAllowed);
//		}
//
//		std::atomic<int32_t> m_count;
//	};
//
//	class ThrowHandler : public HttpHandler
//	{
//	public:
//		virtual HttpResponse handle(const HttpRequest& request)
//		{
//			(void)request;
//			throw 1;
//		}
//	};
//
//	class ReplaceHandler : public HttpHandler
//	{
//	public:
//		explicit ReplaceHandler(const std::string& text) :
//		m_text(text)
//		{
//
//		}
//
//		virtual HttpResponse handle(const HttpRequest& request)
//		{
//			(void)request;
//			return HttpResponse::text(m_text);
//		}
//
//		std::string m_text;
//	};
//
//	class LargeHandler : public HttpHandler
//	{
//	public:
//		virtual HttpResponse handle(const HttpRequest& request)
//		{
//			(void)request;
//			return HttpResponse::text(std::string(200000, 'L'));
//		}
//	};
//
//	class LegacyInterface : public HttpInterface
//	{
//	public:
//		virtual bool httpGetReceive(const std::string& request)
//		{
//			writeSuccess();
//			writeResult("LEGACY_GET:" + request);
//			return true;
//		}
//
//		virtual bool httpPostReceive(const std::string& request)
//		{
//			writeSuccess();
//			writeResult("LEGACY_POST:" + request);
//			return true;
//		}
//	};
//
//	if (argc >= 3 && argv != nullptr && argv[1] != nullptr && std::string(argv[1]) == "--httpservice-child")
//	{
//		int32_t childPortValue = argv[2] == nullptr ? 0 : static_cast<int32_t>(std::atoi(argv[2]));
//		if (childPortValue <= 0)
//		{
//			return 2;
//		}
//
//		std::shared_ptr<TestHandler> childHandler(new TestHandler);
//		HttpService childService;
//		if (!childService.addHandler("/child", childHandler))
//		{
//			return 3;
//		}
//		HttpServiceOptions childOptions;
//		childOptions.m_listenHost = "127.0.0.1";
//		childOptions.m_port = static_cast<uint16_t>(childPortValue);
//		childOptions.m_workerThreads = 4;
//		childOptions.m_enableKeepAlive = false;
//		childOptions.m_enableCors = true;
//		childOptions.m_requestTimeoutMs = 3000;
//		childOptions.m_maxRequestBodyBytes = 128;
//		if (!childService.start(childOptions))
//		{
//			return 4;
//		}
//
//		bool childOk = true;
//		for (int32_t i = 0; i < 10; ++i)
//		{
//			int32_t childStatusCode = 0;
//			std::string childBody;
//			if (!httpRequestByPort(childOptions.m_port,
//				"GET /child?name=process HTTP/1.0\r\nHost: 127.0.0.1\r\n\r\n",
//				childStatusCode,
//				childBody) ||
//				childStatusCode != kHttpStatusOk ||
//				childBody != "GET:process")
//			{
//				childOk = false;
//				break;
//			}
//		}
//		childService.clearHandlers();
//		return childOk ? 0 : 5;
//	}
//
//	HttpRequest request;
//	check(request.m_method.empty() && request.m_remotePort == 0 && request.m_contentLength == 0, "HttpRequest default value");
//	HttpResponse okResponse = HttpResponse::json("{\"ok\":true}");
//	check(okResponse.m_statusCode == kHttpStatusOk && okResponse.m_contentType.find("json") != std::string::npos, "HttpResponse json");
//	HttpResponse textResponse = HttpResponse::text("hello", kHttpStatusCreated);
//	check(textResponse.m_statusCode == kHttpStatusCreated && textResponse.m_body == "hello", "HttpResponse text");
//	textResponse.setHeader("X-Test", "1");
//	check(textResponse.m_headers["X-Test"] == "1", "HttpResponse setHeader");
//	check(HttpServiceHelper::hasHeader(textResponse.m_headers, "x-test"), "HttpServiceHelper hasHeader ignore case");
//	textResponse.setHeader("", "bad");
//	check(textResponse.m_headers.find("") == textResponse.m_headers.end(), "HttpResponse ignore empty header name");
//	check(HttpResponse::defaultStatusText(kHttpStatusNotFound) == "Not Found", "HttpResponse status text");
//	HttpServiceOptions defaultOptions;
//	check(defaultOptions.m_port == 0 && defaultOptions.m_workerThreads > 0, "HttpServiceOptions default");
//	HandlerContext emptyContext;
//	check(emptyContext(nullptr) == 0, "HandlerContext empty callback");
//
//	HttpService invalidService;
//	check(!invalidService.addHandler("", std::shared_ptr<HttpHandler>()), "HttpService add invalid shared handler");
//	check(!invalidService.addHandler("/raw_null", static_cast<HttpHandler*>(nullptr)), "HttpService add invalid raw handler");
//	HttpServiceOptions invalidOptions;
//	check(!invalidService.start(invalidOptions), "HttpService start invalid port");
//
//	TestHandler rawHandler;
//	HttpService rawService;
//	check(rawService.addHandler("/raw", &rawHandler), "HttpService raw handler compatibility");
//	rawService.clearHandlers();
//
//	std::shared_ptr<TestHandler> handler(new TestHandler);
//	std::weak_ptr<TestHandler> handlerWatcher = handler;
//	std::shared_ptr<ThrowHandler> throwHandler(new ThrowHandler);
//	std::shared_ptr<ReplaceHandler> replaceHandlerA(new ReplaceHandler("replace_a"));
//	std::shared_ptr<ReplaceHandler> replaceHandlerB(new ReplaceHandler("replace_b"));
//	std::shared_ptr<LargeHandler> largeHandler(new LargeHandler);
//	std::shared_ptr<LegacyInterface> legacyInterface(new LegacyInterface);
//	HttpService service;
//	check(service.handleContextConnection(nullptr, handler) == 0, "HttpService null context connection");
//	check(service.addHandler("/test", handler), "HttpService add shared handler");
//	handler.reset();
//	check(!handlerWatcher.expired(), "HttpService shared handler retained before start");
//	check(service.addHandler("/throw", throwHandler), "HttpService add throw handler");
//	check(service.addHandler("/replace", replaceHandlerA), "HttpService add replace handler");
//	check(service.addHandler("/large", largeHandler), "HttpService add large handler");
//	check(service.addHandler("/legacy", legacyInterface), "HttpService add legacy interface");
//	HttpServiceOptions options;
//	options.m_listenHost = "127.0.0.1";
//	options.m_workerThreads = 0;
//	options.m_enableKeepAlive = false;
//	options.m_enableCors = true;
//	options.m_requestTimeoutMs = 0;
//	options.m_maxRequestBodyBytes = 64;
//	options.m_port = static_cast<uint16_t>(envInt("HTTPSERVICE_TEST_PORT", 58207));
//	bool started = false;
//	for (int32_t offset = 0; offset < 20; ++offset)
//	{
//		options.m_port = static_cast<uint16_t>(envInt("HTTPSERVICE_TEST_PORT", 58207) + offset);
//		if (service.start(options))
//		{
//			started = true;
//			break;
//		}
//	}
//	check(started && service.isRunning(), "HttpService start");
//	if (!started)
//	{
//		std::cout << "HttpService test FAIL, total=" << totalCount << ", failed=" << failCount << std::endl;
//		return 1;
//	}
//
//	std::function<bool(const std::string&, int32_t&, std::string&)> httpRequest =
//		[&options, &httpRequestByPort](const std::string& requestText, int32_t& statusCode, std::string& body) -> bool
//	{
//		return httpRequestByPort(options.m_port, requestText, statusCode, body);
//	};
//
//	int32_t statusCode = 0;
//	std::string body;
//	check(httpRequest("GET /test?name=cook%20test HTTP/1.0\r\nHost: 127.0.0.1\r\n\r\n", statusCode, body) &&
//		statusCode == kHttpStatusOk && body == "GET:cook test", "HttpService GET");
//	check(httpRequest("POST /test HTTP/1.0\r\nHost: 127.0.0.1\r\nContent-Type: text/plain\r\nContent-Length: 11\r\n\r\nhello world", statusCode, body) &&
//		statusCode == kHttpStatusOk && body.find("hello world") != std::string::npos, "HttpService POST");
//	std::string exactBody(64, 'y');
//	std::ostringstream exactRequest;
//	exactRequest << "POST /test HTTP/1.0\r\nHost: 127.0.0.1\r\nContent-Length: " << exactBody.size() << "\r\n\r\n" << exactBody;
//	check(httpRequest(exactRequest.str(), statusCode, body) &&
//		statusCode == kHttpStatusOk && body.find(exactBody) != std::string::npos, "HttpService payload exact limit");
//	check(httpRequest("GET /test?name=a%ZZ+b%2F HTTP/1.0\r\nHost: 127.0.0.1\r\n\r\n", statusCode, body) &&
//		statusCode == kHttpStatusOk && body == "GET:a%ZZ b/", "HttpService GET query decode edge");
//	check(httpRequest("GET /test?name=edge% HTTP/1.0\r\nHost: 127.0.0.1\r\n\r\n", statusCode, body) &&
//		statusCode == kHttpStatusOk && body == "GET:edge%", "HttpService GET query trailing percent");
//	check(httpRequest("DELETE /test HTTP/1.0\r\nHost: 127.0.0.1\r\n\r\n", statusCode, body) &&
//		statusCode == kHttpStatusMethodNotAllowed, "HttpService method not allowed");
//	std::string largeBody(128, 'x');
//	std::ostringstream largeRequest;
//	largeRequest << "POST /test HTTP/1.0\r\nHost: 127.0.0.1\r\nContent-Length: " << largeBody.size() << "\r\n\r\n" << largeBody;
//	check(httpRequest(largeRequest.str(), statusCode, body) && statusCode == kHttpStatusPayloadTooLarge, "HttpService payload too large");
//	check(httpRequest("OPTIONS /test HTTP/1.0\r\nHost: 127.0.0.1\r\n\r\n", statusCode, body) &&
//		statusCode == kHttpStatusNoContent, "HttpService OPTIONS");
//	check(httpRequest("GET /missing HTTP/1.0\r\nHost: 127.0.0.1\r\n\r\n", statusCode, body) &&
//		statusCode == kHttpStatusNotFound, "HttpService not found");
//	check(httpRequest("GET /throw HTTP/1.0\r\nHost: 127.0.0.1\r\n\r\n", statusCode, body) &&
//		statusCode == kHttpStatusInternalServerError, "HttpService handler exception");
//	check(httpRequest("GET /large HTTP/1.0\r\nHost: 127.0.0.1\r\n\r\n", statusCode, body) &&
//		statusCode == kHttpStatusOk && body.size() == 200000, "HttpService large response");
//	check(httpRequest("GET /replace HTTP/1.0\r\nHost: 127.0.0.1\r\n\r\n", statusCode, body) &&
//		statusCode == kHttpStatusOk && body == "replace_a", "HttpService replace route before update");
//	check(service.addHandler("/replace", replaceHandlerB), "HttpService replace route when running");
//	check(httpRequest("GET /replace HTTP/1.0\r\nHost: 127.0.0.1\r\n\r\n", statusCode, body) &&
//		statusCode == kHttpStatusOk && body == "replace_b", "HttpService replace route after update");
//	check(httpRequest("GET /legacy?x=1 HTTP/1.0\r\nHost: 127.0.0.1\r\n\r\n", statusCode, body) &&
//		statusCode == kHttpStatusOk && body == "LEGACY_GET:x=1", "HttpInterface legacy GET");
//	check(httpRequest("POST /legacy HTTP/1.0\r\nHost: 127.0.0.1\r\nContent-Length: 3\r\n\r\nabc", statusCode, body) &&
//		statusCode == kHttpStatusOk && body == "LEGACY_POST:abc", "HttpInterface legacy POST");
//
//	std::atomic<int32_t> threadFailCount(0);
//	std::vector<std::thread> threads;
//	for (int32_t threadIndex = 0; threadIndex < 8; ++threadIndex)
//	{
//		threads.push_back(std::thread([threadIndex, &httpRequest, &threadFailCount]() -> void
//		{
//			for (int32_t loopIndex = 0; loopIndex < 30; ++loopIndex)
//			{
//				std::ostringstream requestStream;
//				requestStream << "GET /test?name=t" << threadIndex << "_" << loopIndex << " HTTP/1.0\r\nHost: 127.0.0.1\r\n\r\n";
//				int32_t status = 0;
//				std::string responseBody;
//				if (!httpRequest(requestStream.str(), status, responseBody) ||
//					status != kHttpStatusOk ||
//					responseBody.find("GET:t") != 0)
//				{
//					threadFailCount.fetch_add(1);
//				}
//			}
//		}));
//	}
//	for (size_t i = 0; i < threads.size(); ++i)
//	{
//		threads[i].join();
//	}
//	check(threadFailCount.load() == 0, "HttpService multithread pressure");
//	std::shared_ptr<TestHandler> retainedHandler = handlerWatcher.lock();
//	check(retainedHandler.get() != nullptr && retainedHandler->m_count.load() >= 246, "HttpService handler count");
//	retainedHandler.reset();
//
//	std::atomic<int32_t> processFailCount(0);
//	std::vector<std::thread> processThreads;
//	std::string exePath = (argc > 0 && argv != nullptr && argv[0] != nullptr) ? argv[0] : CSystem::GetCurrentExeFullName();
//	auto quoteCommandArgument = [](const std::string& text) -> std::string
//	{
//		std::string result = "\"";
//		for (size_t i = 0; i < text.size(); ++i)
//		{
//			if (text[i] == '"')
//			{
//				result += "\\\"";
//			}
//			else
//			{
//				result.push_back(text[i]);
//			}
//		}
//		result += "\"";
//		return result;
//	};
//	for (int32_t processIndex = 0; processIndex < 4; ++processIndex)
//	{
//		processThreads.push_back(std::thread([processIndex, &processFailCount, &exePath, &quoteCommandArgument, &options]() -> void
//		{
//			if (exePath.empty())
//			{
//				processFailCount.fetch_add(1);
//				return;
//			}
//			uint16_t processPort = static_cast<uint16_t>(options.m_port + 100 + processIndex);
//			std::string command = quoteCommandArgument(exePath) + " --httpservice-child " + CStringManager::toStringInt32(processPort);
//			int32_t result = std::system(command.c_str());
//			if (result != 0)
//			{
//				processFailCount.fetch_add(1);
//			}
//		}));
//	}
//	for (size_t i = 0; i < processThreads.size(); ++i)
//	{
//		processThreads[i].join();
//	}
//	check(processFailCount.load() == 0, "HttpService multiprocess pressure");
//
//	std::shared_ptr<LegacyInterface> runInterface(new LegacyInterface);
//	std::weak_ptr<LegacyInterface> runInterfaceWatcher = runInterface;
//	std::vector<std::pair<std::string, std::shared_ptr<HttpInterface>>> runInterfaces;
//	runInterfaces.push_back(std::make_pair(std::string("/run"), runInterface));
//	runInterface.reset();
//	HttpService runService;
//	bool runStarted = false;
//	for (int32_t offset = 40; offset < 60; ++offset)
//	{
//		runService.run(static_cast<uint16_t>(options.m_port + offset), runInterfaces);
//		if (runService.isRunning())
//		{
//			runStarted = true;
//			break;
//		}
//	}
//	check(runStarted, "HttpService shared interface run");
//	check(!runInterfaceWatcher.expired(), "HttpService shared interface retained after caller reset");
//	runInterfaces.clear();
//	check(!runInterfaceWatcher.expired(), "HttpService shared interface retained by route");
//	runService.clearHandlers();
//	check(runInterfaceWatcher.expired(), "HttpService shared interface released after clearHandlers");
//
//	service.stop();
//	check(!service.isRunning(), "HttpService stop");
//	check(!handlerWatcher.expired(), "HttpService shared handler retained after stop");
//	service.clearHandlers();
//	check(handlerWatcher.expired(), "HttpService shared handler released after clearHandlers");
//	std::cout << "HttpService test " << (failCount == 0 ? "PASS" : "FAIL") <<
//		", total=" << totalCount << ", failed=" << failCount << std::endl;
//	return failCount == 0 ? 0 : 1;
//}