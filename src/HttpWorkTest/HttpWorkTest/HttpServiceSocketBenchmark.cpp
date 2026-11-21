#include "HttpWorkTest.h"
#include <cstring>
#include <stdexcept>
#include <string>
#ifdef _WIN32
#include <windows.h>
#ifndef _WINSOCK2API_
#include <winsock.h>
#endif
typedef SOCKET BenchmarkSocket;
static const BenchmarkSocket kInvalidSocket = INVALID_SOCKET;
#else
#include <arpa/inet.h>
#include <cerrno>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
typedef int BenchmarkSocket;
static const BenchmarkSocket kInvalidSocket = -1;
#endif
#include "PlatformTest/PlatformTestAPI.h"

/** 建立回环连接，读取响应直到服务器关闭连接
@param [in] port 已注册 /test 处理器的回环端口
@param [in] request 以零结尾的请求文本
@param [out] answer 当前连接的完整响应字节
@return 正常收到 EOF 且未超限时返回 true
*/
static bool SocketExchange(uint16_t port, const char* request, std::string& answer)
{
    const BenchmarkSocket client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client == kInvalidSocket)
    {
        return false;
    }
    sockaddr_in address = {};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
#ifdef _WIN32
    const int timeout = 5000;
#else
    const timeval timeout = {5, 0};
#endif
    bool ok = setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout)) == 0 &&
        setsockopt(client, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout)) == 0 &&
        connect(client, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) == 0;
    const int requestSize = static_cast<int>(std::strlen(request));
    int sent = 0;
    while (ok && sent < requestSize)
    {
#ifdef __linux__
        const int flags = MSG_NOSIGNAL;
#else
        const int flags = 0;
#endif
        const int count = static_cast<int>(send(client, request + sent, requestSize - sent, flags));
        ok = count > 0;
        if (ok)
        {
            sent += count;
        }
    }
    answer.clear();
    char buffer[2048];
    while (ok)
    {
        const int count = static_cast<int>(recv(client, buffer, sizeof(buffer), 0));
        if (count <= 0)
        {
            ok = count == 0;
            break;
        }
        answer.append(buffer, static_cast<size_t>(count));
        ok = answer.size() <= 16384;
    }
#ifdef _WIN32
    closesocket(client);
#else
    close(client);
#endif
    return ok;
}

bool CheckHttpServiceBodyless(uint16_t port, const char* request, int32_t status, int32_t length)
{
    std::string answer;
    if (!SocketExchange(port, request, answer)) { return false; }
    const size_t end = answer.find("\r\n\r\n");
    const std::string prefix = "HTTP/1.1 " + std::to_string(status) + " ";
    const std::string lengthHeader = "\r\nContent-Length: ";
    return end != std::string::npos && answer.size() == end + 4 && answer.compare(0, prefix.size(), prefix) == 0 &&
        (length < 0 ? answer.find(lengthHeader) == std::string::npos :
            answer.find(lengthHeader + std::to_string(length) + "\r\n") != std::string::npos);
}

/** 核对 GET 的实际状态码及固定九字节正文
@param [in] port 回环测试服务端口
@return 收到完整的预期响应时返回 true
*/
static bool SocketGet(uint16_t port)
{
    std::string answer;
    if (!SocketExchange(port, "GET /test?name=bench HTTP/1.0\r\nHost: 127.0.0.1\r\n\r\n", answer)) { return false; }
    const size_t end = answer.find("\r\n\r\n");
    return end != std::string::npos && answer.size() == end + 4 + 9 &&
        answer.compare(0, 9, "HTTP/1.1 ") == 0 && answer.compare(9, 4, "200 ") == 0 &&
        answer.compare(end + 4, 9, "GET:bench") == 0;
}

void RunHttpServiceSocketBenchmark(uint16_t port)
{
    // HttpService 已执行启动过程，Windows 当前进程的 Winsock 在 stop 之前有效。
    LibraryBenchmark::run("HttpService.GET.loopback.socket", [port](uint32_t count) -> uint64_t {
        for (uint32_t i = 0; i < count; ++i)
        {
            if (!SocketGet(port))
            {
                throw std::runtime_error("HttpService socket benchmark failed");
            }
        }
        return 9ULL * count;
    });
}