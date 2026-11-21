#include "HttpFixtureOrigin.h"
#include <mbedtls/ssl.h>
#include <mbedtls/x509_crt.h>
#include <algorithm>
#include <chrono>
#include <climits>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>
#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <bcrypt.h>
typedef SOCKET FixtureSocket;
#define FIXTURE_INVALID INVALID_SOCKET
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>
typedef int FixtureSocket;
#define FIXTURE_INVALID (-1)
#endif

/** 关闭一次测试拥有的套接字
*/
static void CloseSocket(FixtureSocket socket)
{
#ifdef _WIN32
    closesocket(socket);
#else
    close(socket);
#endif
}

/** 设置套接字阻塞模式；监听套接字非阻塞，已接受的连接使用阻塞超时
*/
static bool Blocking(FixtureSocket socket, bool block)
{
#ifdef _WIN32
    u_long value = block ? 0 : 1;
    return ioctlsocket(socket, FIONBIO, &value) == 0;
#else
    int flags = fcntl(socket, F_GETFL);
    return flags >= 0 && fcntl(socket, F_SETFL, block ? flags & ~O_NONBLOCK : flags | O_NONBLOCK) == 0;
#endif
}

/** 从宿主操作系统为单连接 TLS 提供独立密码学随机字节
*/
static int Entropy(void*, unsigned char* output, size_t size)
{
#ifdef _WIN32
    return size <= ULONG_MAX && BCryptGenRandom(nullptr, output, static_cast<ULONG>(size), BCRYPT_USE_SYSTEM_PREFERRED_RNG) == 0 ? 0 : -1;
#else
    std::ifstream random("/dev/urandom", std::ios::binary);
    return random.read(reinterpret_cast<char*>(output), size) ? 0 : -1;
#endif
}

/** 将 TLS 记录发给单个连接，错误结束该次请求
*/
static int Send(void* context, const unsigned char* data, size_t size)
{
    return static_cast<int>(send(*static_cast<FixtureSocket*>(context), reinterpret_cast<const char*>(data), static_cast<int>((std::min)(size, static_cast<size_t>(INT_MAX))), 0));
}

/** 从单个连接接收至多一个 TLS 缓冲区的数据，返回实收字节
*/
static int Receive(void* context, unsigned char* data, size_t size)
{
    return static_cast<int>(recv(*static_cast<FixtureSocket*>(context), reinterpret_cast<char*>(data), static_cast<int>((std::min)(size, static_cast<size_t>(INT_MAX))), 0));
}

/** 将请求头解析为小写名称的映射，值保留 ETag 引号或 Range 的原始内容
*/
static std::map<std::string, std::string> Headers(const std::string& raw)
{
    std::map<std::string, std::string> headers;
    std::istringstream input(raw);
    std::string line;
    std::getline(input, line);
    while (std::getline(input, line))
    {
        size_t colon = line.find(':');
        if (colon == std::string::npos)
        {
            continue;
        }
        std::string name = line.substr(0, colon);
        for (size_t i = 0; i < name.size(); ++i)
        {
            name[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(name[i])));
        }
        size_t start = line.find_first_not_of(" \t\r", colon + 1);
        headers[name] = start == std::string::npos ? "" : line.substr(start, line.find_last_not_of(" \t\r") - start + 1);
    }
    return headers;
}

HttpFixtureOrigin::HttpFixtureOrigin(unsigned port, const std::string& certificate, const std::string& key) :
    m_listener(FIXTURE_INVALID), m_port(0), m_certificate(certificate), m_key(key), m_stopping(false), m_posts(0), m_connections(0), m_heldCount(0), m_open(false)
{
#ifdef _WIN32
    WSADATA network;
    if (WSAStartup(MAKEWORD(2, 2), &network) != 0)
    {
        throw std::runtime_error("WSAStartup failed");
    }
#else
    signal(SIGPIPE, SIG_IGN);
#endif
    FixtureSocket listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in address = {};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(static_cast<unsigned short>(port));
#ifdef _WIN32
    int length = sizeof(address);
#else
    socklen_t length = sizeof(address);
#endif
    try
    {
        if (port > 65535 || listener == FIXTURE_INVALID || bind(listener, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0 ||
            listen(listener, 32) != 0 || getsockname(listener, reinterpret_cast<sockaddr*>(&address), &length) != 0 || !Blocking(listener, false))
        {
            throw std::runtime_error("Cannot start HTTP fixture on loopback");
        }
        m_port = ntohs(address.sin_port);
        m_listener = listener;
        m_workers.reserve(8);
        for (unsigned i = 0; i < 8; ++i)
        {
            m_workers.emplace_back(&HttpFixtureOrigin::serve, this);
        }
    }
    catch (...)
    {
        m_stopping = true;
        for (size_t i = 0; i < m_workers.size(); ++i)
        {
            m_workers[i].join();
        }
        if (listener != FIXTURE_INVALID)
        {
            CloseSocket(listener);
        }
#ifdef _WIN32
        WSACleanup();
#endif
        throw;
    }
}

HttpFixtureOrigin::~HttpFixtureOrigin()
{
    m_stopping = true;
    m_released.notify_all();
    for (size_t i = 0; i < m_workers.size(); ++i)
    {
        m_workers[i].join();
    }
    CloseSocket(static_cast<FixtureSocket>(m_listener));
#ifdef _WIN32
    WSACleanup();
#endif
}

unsigned HttpFixtureOrigin::port() const
{
    return m_port;
}

unsigned HttpFixtureOrigin::posts() const
{
    return m_posts.load();
}

void HttpFixtureOrigin::serve()
{
    FixtureSocket listener = static_cast<FixtureSocket>(m_listener);
    while (!m_stopping)
    {
        fd_set pending;
        FD_ZERO(&pending);
        FD_SET(listener, &pending);
        timeval wait = {0, 150000};
        if (select(static_cast<int>(listener + 1), &pending, nullptr, nullptr, &wait) <= 0)
        {
            continue;
        }
        FixtureSocket client = accept(listener, nullptr, nullptr);
        if (client == FIXTURE_INVALID)
        {
            continue;
        }
        Blocking(client, true);
        const uint64_t identity = ++m_connections;
#ifdef _WIN32
        int timeout = 6000;
#else
        timeval timeout = {6, 0};
#endif
        setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
        setsockopt(client, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
        mbedtls_ssl_context ssl;
        mbedtls_ssl_config config;
        mbedtls_x509_crt cert;
        mbedtls_pk_context key;
        mbedtls_ssl_init(&ssl);
        mbedtls_ssl_config_init(&config);
        mbedtls_x509_crt_init(&cert);
        mbedtls_pk_init(&key);
        bool tls = !m_certificate.empty();
        bool ready = true;
        if (tls)
        {
            ready = mbedtls_ssl_config_defaults(&config, MBEDTLS_SSL_IS_SERVER, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT) == 0 &&
                mbedtls_x509_crt_parse(&cert, reinterpret_cast<const unsigned char*>(m_certificate.c_str()), m_certificate.size() + 1) == 0 &&
                mbedtls_pk_parse_key(&key, reinterpret_cast<const unsigned char*>(m_key.c_str()), m_key.size() + 1, nullptr, 0, Entropy, nullptr) == 0 &&
                mbedtls_ssl_conf_own_cert(&config, &cert, &key) == 0;
            mbedtls_ssl_conf_rng(&config, Entropy, nullptr);
            ready = ready && mbedtls_ssl_setup(&ssl, &config) == 0;
            mbedtls_ssl_set_bio(&ssl, &client, Send, Receive, nullptr);
            ready = ready && mbedtls_ssl_handshake(&ssl) == 0;
        }
        try
        {
            std::string request;
            size_t separator = std::string::npos;
            size_t length = 0;
            unsigned char buffer[4096];
            while (ready)
            {
                int got = tls ? mbedtls_ssl_read(&ssl, buffer, sizeof(buffer)) : Receive(&client, buffer, sizeof(buffer));
                if (got <= 0)
                {
                    break;
                }
                request.append(reinterpret_cast<char*>(buffer), got);
                if (separator == std::string::npos)
                {
                    separator = request.find("\r\n\r\n");
                    if (separator == std::string::npos)
                    {
                        if (request.size() > 65536)
                        {
                            break;
                        }
                        continue;
                    }
                    std::string field = Headers(request.substr(0, separator))["content-length"];
                    if (field.size() > 7 || field.find_first_not_of("0123456789") != std::string::npos)
                    {
                        break;
                    }
                    length = static_cast<size_t>(std::atoi(field.c_str()));
                    if (length > 1024 * 1024)
                    {
                        break;
                    }
                }
                if (request.size() >= separator + 4 + length)
                {
                    const std::string answer = response(request.substr(0, separator), request.substr(separator + 4, length), identity);
                    size_t sent = 0;
                    while (sent < answer.size())
                    {
                        const unsigned char* data = reinterpret_cast<const unsigned char*>(answer.data() + sent);
                        int count = tls ? mbedtls_ssl_write(&ssl, data, (std::min)(answer.size() - sent, static_cast<size_t>(8192))) : Send(&client, data, answer.size() - sent);
                        if (count <= 0)
                        {
                            break;
                        }
                        sent += count;
                    }
                    if (sent == answer.size() && answer.find("\r\nConnection: keep-alive\r\n") != std::string::npos)
                    {
                        request.clear();
                        separator = std::string::npos;
                        length = 0;
                        continue;
                    }
                    if (tls)
                    {
                        mbedtls_ssl_close_notify(&ssl);
                    }
                    break;
                }
            }
        }
        catch (const std::exception&)
        {
            // 错误只终止当前连接，下一请求仍可执行；测试客户端会记录请求失败。
        }
        mbedtls_ssl_free(&ssl);
        mbedtls_ssl_config_free(&config);
        mbedtls_x509_crt_free(&cert);
        mbedtls_pk_free(&key);
        CloseSocket(client);
    }
}

std::string HttpFixtureOrigin::response(const std::string& raw, const std::string& body, uint64_t identity)
{
    std::istringstream request(raw);
    std::string method, path, status("200 OK"), data, extra;
    request >> method >> path;
    std::map<std::string, std::string> fields = Headers(raw);
    const std::string media("A\0BCD", 5);
    const bool keep = path == "/reuse" || path == "/keep-media";
    if (path == "/reuse")
    {
        if (method == "POST")
        {
            ++m_posts;
        }
        data = std::to_string(m_port) + "/" + std::to_string(identity) + "\n" + method + "\n" +
            fields["range"] + "\n" + fields["if-range"] + "\n" + fields["if-none-match"] + "\n" + fields["content-type"] + "\n" + body;
    }
    else if (path == "/keep-media")
    {
        data = media;
    }
    else if (method == "OPTIONS")
    {
        status = "204 No Content";
        extra = "Access-Control-Allow-Headers: Range, If-Range, If-None-Match, Content-Type\r\n";
    }
    else if (method == "POST")
    {
        ++m_posts; data = body;
    }
    else if (path == "/held")
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        ++m_heldCount;
        if (!m_released.wait_for(lock, std::chrono::seconds(4), [this]() { return m_open || m_stopping; }))
        {
            status = "500 Timeout";
        }
        data = std::to_string(m_heldCount);
    }
    else if (path == "/release")
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_open = true;
        m_released.notify_all();
        status = "204 No Content";
    }
    else if (path == "/redirect")
    {
        status = "302 Found"; extra = "Location: /video/sample\r\n";
    }
    else if (path == "/etag" && fields["if-none-match"] == "\"one\"")
    {
        status = "304 Not Modified";
    }
    else if (path == "/audio/sample" && fields["range"] == "bytes=1-3" && fields["if-range"] == "\"one\"")
    {
        status = "206 Partial Content";
        extra = "Content-Range: bytes 1-3/5\r\nETag: \"one\"\r\n";
        data = media.substr(1, 3);
    }
    else if (path == "/video/sample" && !fields["range"].empty())
    {
        status = "416 Range Not Satisfiable"; extra = "Content-Range: bytes */5\r\n";
    }
    else if (path == "/video/sample")
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_open = false;
        m_heldCount = 0;
        data = media;
    }
    else if (path == "/empty")
    {
        status = "204 No Content";
    }
    else
    {
        status = "404 Not Found"; data = "missing";
    }
    return "HTTP/1.1 " + status + (keep ? "\r\nConnection: keep-alive" : "\r\nConnection: close") +
        "\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Expose-Headers: Content-Range, ETag\r\nContent-Length: " +
        std::to_string(data.size()) + "\r\n" + extra + "\r\n" + data;
}