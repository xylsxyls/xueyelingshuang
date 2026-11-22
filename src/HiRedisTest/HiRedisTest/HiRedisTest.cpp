#include "PlatformTest/PlatformTestAPI.h"
#include "HiRedisTest.h"
#include <cstring>
#include <iterator>
#include <chrono>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#endif
#include "hiredisinclude/hiredis.h"
#include "hiredisinclude/sds.h"
#include <cerrno>
#include <climits>
#include <stdexcept>
#include <algorithm>
#include "HiRedis/HiRedisHelper.h"

#include <atomic>
#include <cstdlib>
#include <ctime>
#include <clocale>
#include <functional>
#include <future>
#include <iostream>
#include <sstream>
#include <thread>

#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

template struct HiRedisContextReader<&HiRedis::m_redis>;

/** 基准输入固定为五个参数，包含 256 字节的值；预先核对解析内容再计时
*/
static void RunBenchmark()
{
    const std::string command = "SET key " + std::string(256, 'a') + " EX 60";
    std::vector<std::string> arguments;
    std::string error;
    const std::vector<std::string> expected = {"SET", "key", std::string(256, 'a'), "EX", "60"};
    if (!HiRedisHelper::splitCommandText(command, arguments, error) || arguments != expected)
    {
        throw std::runtime_error("Unexpected parsed argument values");
    }
    LibraryBenchmark::run("HiRedis", [&](uint32_t count) -> uint64_t
    {
        uint64_t checksum = 0;
        for (uint32_t index = 0; index < count; ++index)
        {
            checksum += HiRedisHelper::splitCommandText(command, arguments, error) ? arguments.size() : 0;
        }
        if (checksum != static_cast<uint64_t>(count) * 5)
        {
            throw std::runtime_error("Unexpected parsed argument count");
        }
        return checksum;
    });
}

/** 比较同一连接上逐条和每批 16 条 PING 的摊销时间，核对每个回显
@param [in] redis 已认证并核实版本的 Redis 连接，计时期间由本测试独占
*/
static void RunDatabaseBenchmark(HiRedis& redis)
{
    const uint32_t width = 16;
    std::vector<HiRedisCommand> commands;
    for (uint32_t i = 0; i < width; ++i)
    {
        const std::string echo(1, static_cast<char>('A' + i));
        commands.push_back(HiRedisCommand(std::vector<std::string>{"PING", echo}));
    }
    LibraryBenchmark::run("HiRedis.Ping", [&](uint32_t count) -> uint64_t
    {
        uint64_t checksum = 0;
        for (uint32_t i = 0; i < count; ++i)
        {
            const std::vector<std::string>& args = commands[i % width].m_args;
            const std::shared_ptr<HiRedisResultSet> reply = redis.command(args);
            if (!reply || reply->isError() || reply->toString() != args[1])
            {
                throw std::runtime_error("Unexpected Redis PING reply");
            }
            checksum += static_cast<unsigned char>(args[1][0]);
        }
        return checksum;
    });
    LibraryBenchmark::run("HiRedis.PipelinePing16", [&](uint32_t count) -> uint64_t
    {
        uint64_t checksum = 0;
        for (uint32_t i = 0; i < count; ++i)
        {
            const std::vector<std::shared_ptr<HiRedisResultSet>> replies = redis.pipeline(commands);
            if (replies.size() != width)
            {
                throw std::runtime_error("Unexpected Redis pipeline reply count");
            }
            for (uint32_t j = 0; j < width; ++j)
            {
                const std::string& echo = commands[j].m_args[1];
                if (!replies[j] || replies[j]->isError() || replies[j]->toString() != echo)
                {
                    throw std::runtime_error("Unexpected Redis pipeline reply");
                }
                checksum += static_cast<unsigned char>(echo[0]);
            }
        }
        return checksum;
    }, width);
}

/** 解析测试实例的整数环境变量，拒绝截断、溢出及尾随字符
@param [in] name 环境变量名
@param [in,out] number 未指定时保留默认值
@return 是否缺省或成功解析
*/
static bool ReadNumber(const char* name, int32_t& number)
{
    const char* text = std::getenv(name);
    if (!text || !*text)
    {
        return true;
    }
    char* end = nullptr;
    errno = 0;
    const long value = std::strtol(text, &end, 10);
    if (errno == ERANGE || end == text || *end || value < INT32_MIN || value > INT32_MAX)
    {
        std::cerr << "Invalid " << name << std::endl;
        return false;
    }
    number = static_cast<int32_t>(value);
    return true;
}

/** 读取本批测试实例的配置
@param [out] config 接收主机、端口、密码、数据库和超时
@return 所有数值输入是否有效
*/
static bool ReadConfig(HiRedisConfig& config)
{
    int32_t port = config.m_port, db = config.m_dbIndex;
    int32_t connectMs = config.m_connectTimeoutMs, commandMs = config.m_commandTimeoutMs;
    if (!ReadNumber("HIREDIS_TEST_PORT", port) || !ReadNumber("HIREDIS_TEST_DB", db) ||
        !ReadNumber("HIREDIS_TEST_CONNECT_TIMEOUT_MS", connectMs) ||
        !ReadNumber("HIREDIS_TEST_COMMAND_TIMEOUT_MS", commandMs))
    {
        return false;
    }
    const char* host = std::getenv("HIREDIS_TEST_IP");
    const char* password = std::getenv("HIREDIS_TEST_PASSWORD");
    config.setEndpoint(host ? host : config.m_ip, port);
    config.setPassword(password ? password : "");
    config.setDatabase(db);
    config.setTimeouts(connectMs, commandMs);
    return true;
}

/** 在独占测试连接上核对 hiredis 保存的建连期限、套接字超时及保活标记
@param [in] redis 已就绪且无并发访问的连接
@param [in] connectMs 期望的建连超时毫秒数
@param [in] commandMs 期望的读写超时毫秒数
@param [in] keepAlive 期望的保活开关
@return 底层状态是否符合预期
*/
static bool SocketSettings(HiRedis& redis, int32_t connectMs, int32_t commandMs, bool keepAlive)
{
    const redisContext* context = redis.*ContextMember(HiRedisContextMember());
    if (!context || !context->connect_timeout ||
        context->connect_timeout->tv_sec * 1000 + context->connect_timeout->tv_usec / 1000 != connectMs)
    {
        return false;
    }
    int enabled = 0;
#ifdef _WIN32
    int length = sizeof(enabled);
    DWORD receive = 0, send = 0;
#else
    socklen_t length = sizeof(enabled);
    timeval receive = {}, send = {};
#endif
    if (getsockopt(context->fd, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast<char*>(&enabled), &length) ||
        (enabled != 0) != keepAlive)
    {
        return false;
    }
    length = sizeof(receive);
    if (getsockopt(context->fd, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<char*>(&receive), &length) ||
        getsockopt(context->fd, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<char*>(&send), &length))
    {
        return false;
    }
#ifdef _WIN32
    return receive == static_cast<DWORD>(commandMs) && send == static_cast<DWORD>(commandMs);
#else
    // 整秒输入可在不同内核节拍下精确表示。
    return receive.tv_sec == commandMs / 1000 && !receive.tv_usec &&
        send.tv_sec == commandMs / 1000 && !send.tv_usec;
#endif
}

/** 在已有连接所占的本机非监听端口验证建连失败，不依赖固定端口上的服务
@param [in] active 已连接到测试实例的独占 TCP 客户端
@return 对本机客户端端口的连接失败且保留诊断时返回 true
*/
static bool RejectsUnavailableEndpoint(HiRedis& active)
{
    const auto* context = active.*ContextMember(HiRedisContextMember());
    sockaddr_storage local = {};
#ifdef _WIN32
    int length = sizeof(local);
#else
    socklen_t length = sizeof(local);
#endif
    char host[NI_MAXHOST] = {}, service[NI_MAXSERV] = {};
    if (!context || getsockname(context->fd, reinterpret_cast<sockaddr*>(&local), &length) != 0 ||
        getnameinfo(reinterpret_cast<const sockaddr*>(&local), length, host, sizeof(host),
            service, sizeof(service), NI_NUMERICHOST | NI_NUMERICSERV) != 0)
    {
        return false;
    }
    HiRedisConfig unavailable;
    unavailable.setEndpoint(host, static_cast<int32_t>(std::strtol(service, nullptr, 10)));
    unavailable.setDatabase(-1);
    unavailable.setTimeouts(500, 500);
    HiRedis probe;
    return !probe.init(unavailable) && !probe.isConnected() && !probe.lastError().empty();
}

/** 按独立的服务端 CLIENT ID 观察连接的存在或关闭
@param [in] observer 用于查询 CLIENT LIST 的独立连接
@param [in] id 先前由目标连接读取的正数 CLIENT ID
@param [in] present 为 true 时检查连接存在，为 false 时等待服务端处理关闭
@return 在期限内观察到预期状态时返回 true，查询失败或 ID 无效时返回 false
*/
static bool ClientState(HiRedis& observer, int64_t id, bool present)
{
    if (id <= 0)
    {
        return false;
    }
    const std::string number = std::to_string(id);
    const std::vector<std::string> query = {"CLIENT", "LIST", "ID", number};
    const std::string prefix = "id=" + number + " ";
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    do
    {
        const auto reply = observer.command(query);
        if (!reply || reply->isError() || reply->type() != REDIS_REPLY_STRING)
        {
            return false;
        }
        const std::string line = reply->toString();
        if (present ? line.compare(0, prefix.size(), prefix) == 0 : line.empty())
        {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    } while (std::chrono::steady_clock::now() < deadline);
    return false;
}

/** 验证配置快照、显式认证以及成功和失败的选库对重连的影响
@return 连接身份、认证、逻辑库和套接字设置与独立预期一致时返回 0
*/
static int RunSessionSettings()
{
    HiRedisConfig config;
    if (!ReadConfig(config) || config.m_password.empty())
    {
        return 1;
    }
    config.setDatabase(1);
    config.setTimeouts(1537, 2000);
    config.setKeepAlive(false);
    HiRedis redis;
    if (!redis.init(config))
    {
        return 1;
    }
    const int64_t firstId = redis.command("CLIENT ID")->toInt64();
    const std::string password = config.m_password;
    config.setPassword(password + "_changed");
    config.setDatabase(0);
    config.setTimeouts(3000, 3000);
    config.setKeepAlive(true);
    if (firstId <= 0 || redis.command("CLIENT INFO")->toString().find(" db=1 ") == std::string::npos ||
        redis.command("RESET")->toString() != "RESET" || redis.ping() || redis.lastError().find("NOAUTH") == std::string::npos ||
        !redis.auth(password) || !redis.selectDbIndex(2) || redis.selectDb(-1) || redis.lastError().empty())
    {
        std::cerr << "Authentication or database selection had an unexpected result" << std::endl;
        return 1;
    }
    if (!redis.reconnect() || !SocketSettings(redis, 1537, 2000, false))
    {
        std::cerr << "Failed to restore connection settings: " << redis.lastError() << std::endl;
        return 1;
    }
    const int64_t nextId = redis.command("CLIENT ID")->toInt64();
    if (nextId <= 0 || nextId == firstId || redis.command("CLIENT INFO")->toString().find(" db=2 ") == std::string::npos)
    {
        return 1;
    }

    // 建连时不保存任何密码，重连成功只能来自随后成功的 auth。
    config.setPassword("");
    config.setDatabase(-1);
    HiRedis session;
    if (!session.init(config) || session.ping() || session.lastError().find("NOAUTH") == std::string::npos ||
        !session.auth(password) || !session.ping())
    {
        std::cerr << "Explicit authentication did not unlock the unauthenticated session" << std::endl;
        return 1;
    }
    const int64_t authenticatedId = session.command("CLIENT ID")->toInt64();
    if (session.auth(password + std::string("\0bad", 4)) || session.lastError().find("WRONGPASS") == std::string::npos ||
        !session.reconnect() || !session.ping())
    {
        std::cerr << "Reconnect did not use the last successful explicit credential" << std::endl;
        return 1;
    }
    const int64_t restoredId = session.command("CLIENT ID")->toInt64();
    return authenticatedId > 0 && restoredId > 0 && restoredId != authenticatedId ? 0 : 1;
}

/** 用相同的服务端阻塞期限，区分客户端读超时和服务端正常返回 NIL
@return 短期限产生传输错误、重连恢复且长期限收到有效 NIL 时返回 0
*/
static int RunBlockingTimeout()
{
    HiRedisConfig config;
    if (!ReadConfig(config))
    {
        return 1;
    }
    config.setTimeouts(3000, 150);
    HiRedis client;
    if (!client.init(config))
    {
        return 1;
    }
    const int64_t firstId = client.command("CLIENT ID")->toInt64();
    const std::string key = "hiredis_test_blpop_" + std::to_string(firstId);
    const auto type = client.command(std::vector<std::string>{"TYPE", key});
    if (firstId <= 0 || type->type() != REDIS_REPLY_STATUS || type->toString() != "none")
    {
        return 1;
    }
    const HiRedisCommand wait({"BLPOP", key, "0.8"});
    const auto expired = client.command(wait);
    const std::string diagnostic = expired->error();
    bool readTimeout = diagnostic == "recv timeout";
#ifndef _WIN32
    // Linux 上阻塞 recv 达到 SO_RCVTIMEO 时可返回 EAGAIN 或 EWOULDBLOCK。
    readTimeout = readTimeout || diagnostic == std::strerror(EAGAIN) || diagnostic == std::strerror(EWOULDBLOCK);
#endif
    if (!expired->isError() || expired->toReply() != nullptr || !readTimeout ||
        client.lastError() != diagnostic || client.isConnected() || !client.reconnect() || !client.ping())
    {
        std::cerr << "Expected a client read timeout followed by successful reconnect" << std::endl;
        return 1;
    }
    const int64_t recoveredId = client.command("CLIENT ID")->toInt64();
    config.setTimeouts(3000, 3000);
    if (recoveredId <= 0 || recoveredId == firstId || !client.init(config))
    {
        return 1;
    }
    const int64_t beforeWait = client.command("CLIENT ID")->toInt64();
    const auto empty = client.command(wait);
    if (!empty->isValid() || empty->type() != REDIS_REPLY_NIL || !client.lastError().empty() ||
        !client.isConnected() || !client.ping())
    {
        std::cerr << "Expected a Redis NIL reply without losing the connection" << std::endl;
        return 1;
    }
    return beforeWait > 0 && client.command("CLIENT ID")->toInt64() == beforeWait ? 0 : 1;
}

/** 验证初始化配置的快照、底层选项、选库及默认借用期限
@return 所有设置生效时返回 0
*/
static int RunSettings()
{
    HiRedisConfig config;
    if (!ReadConfig(config))
    {
        return 1;
    }
    // 使用独立的非零预期，遗漏 SELECT 或空实现的设置函数都不能通过。
    config.setDatabase(1);
    HiRedis redis;
    for (int i = 0; i < 4; ++i)
    {
        // 两项超时分别覆盖显式值和默认回退，防止一项非正数将另一项一并覆盖。
        config.setTimeouts((i & 1) ? 0 : 1234, (i & 2) ? -1 : 2000);
        config.setKeepAlive(i != 0);
        if (!redis.init(config) || !SocketSettings(redis, (i & 1) ? 3000 : 1234, (i & 2) ? 3000 : 2000, i != 0))
        {
            std::cerr << "Socket settings failed, mode=" << i << " " << redis.lastError() << std::endl;
            return 1;
        }
        const auto info = redis.command("CLIENT INFO");
        if (info->isError() || info->toString().find(" db=1 ") == std::string::npos)
        {
            std::cerr << "Selected database differs from configuration" << std::endl;
            return 1;
        }
    }
    HiRedis observer;
    if (!observer.init(config))
    {
        return 1;
    }
    config.setDatabase(-1);
    if (!redis.init(config))
    {
        return 1;
    }
    // 先由观察者记录新会话最后执行的命令，再读取新会话 ID。这样能区分跳过 SELECT 和执行 SELECT 0。
    std::istringstream clients(observer.command("CLIENT LIST TYPE normal")->toString());
    const auto id = redis.command("CLIENT ID");
    const std::string prefix = "id=" + id->toString() + " ";
    std::string line;
    bool skippedSelect = false;
    while (std::getline(clients, line))
    {
        if (line.compare(0, prefix.size(), prefix) == 0)
        {
            skippedSelect = line.find(" db=0 ") != std::string::npos && line.find(" cmd=select ") == std::string::npos;
            break;
        }
    }
    if (id->toInt64() <= 0 || !skippedSelect)
    {
        std::cerr << "A negative database must skip SELECT on the new connection" << std::endl;
        return 1;
    }
    HiRedisConnectionPool pool;
    config.setPoolBounds(1, 2);
    config.setPoolAcquireTimeout(40);
    if (!pool.init(config) || pool.availableCount() != 1)
    {
        return 1;
    }
    config.setPoolBounds(0, -1);
    config.setPoolAcquireTimeout(0);
    auto first = pool.acquire(), second = pool.acquire();
    std::chrono::steady_clock::duration elapsed = std::chrono::steady_clock::duration::zero();
    // 只计工作线程实际等待时间；若配置失效而永久阻塞，由主线程关闭池唤醒。
    auto waiting = std::async(std::launch::async, [&]() {
        const auto start = std::chrono::steady_clock::now();
        auto acquired = pool.acquire();
        elapsed = std::chrono::steady_clock::now() - start;
        return acquired;
    });
    if (waiting.wait_for(std::chrono::seconds(2)) != std::future_status::ready)
    {
        pool.close();
        return 1;
    }
    if (!first || !second || waiting.get() || pool.lastError() != "redis pool acquire timeout" ||
        elapsed < std::chrono::milliseconds(35) || elapsed >= std::chrono::seconds(2))
    {
        std::cerr << "Pool did not retain its configured bounds and wait time" << std::endl;
        return 1;
    }
    first.reset();
    second.reset();
    if (pool.availableCount() != 1 || pool.totalCount() != 1)
    {
        return 1;
    }
    pool.close();
    if (!pool.init(config) || pool.totalCount() != 0)
    {
        return 1;
    }
    first = pool.acquire(0);
    second = pool.acquire(0);
    auto third = pool.acquire(0);
    if (!first || !second || !third || pool.totalCount() != 3)
    {
        return 1;
    }
    first.reset();
    second.reset();
    third.reset();
    return pool.totalCount() == 0 && pool.availableCount() == 0 ? 0 : 1;
}

/** 借用方临时使用的错误凭据不能破坏池的下一次建连
@return 新会话的身份、配置及池计数均符合预期时返回 0
*/
static int RunPoolRecovery()
{
    HiRedisConfig config;
    if (!ReadConfig(config))
    {
        return 1;
    }
    config.setDatabase(1);
    config.setTimeouts(1234, 2000);
    config.setKeepAlive(false);
    config.setReaderLimits(4096, 3);
    HiRedisConnectionPool pool;
    if (!pool.init(config, 1))
    {
        std::cerr << pool.lastError() << std::endl;
        return 1;
    }
    auto borrowed = pool.acquire(0);
    if (!borrowed)
    {
        return 1;
    }
    const int64_t originalId = borrowed->command("CLIENT ID")->toInt64();
    config.setPassword(config.m_password + "_invalid");
    config.setReaderLimits(-1, -1);
    if (originalId <= 0 || borrowed->init(config) || borrowed->isConnected() || borrowed->lastError().empty())
    {
        std::cerr << "Expected failed authentication on the borrowed connection" << std::endl;
        return 1;
    }
    borrowed.reset();
    auto restored = pool.acquire(0);
    if (!restored || !restored->ping() || !SocketSettings(*restored, 1234, 2000, false))
    {
        std::cerr << "Pool configuration was not restored: " << pool.lastError() << std::endl;
        return 1;
    }
    const auto* context = (*restored).*ContextMember(HiRedisContextMember());
    if (!context || !context->reader || context->reader->maxbuf != 4096 || context->reader->maxelements != 3)
    {
        std::cerr << "Pool reader limits differ from the initial configuration" << std::endl;
        return 1;
    }
    const int64_t newId = restored->command("CLIENT ID")->toInt64();
    const auto info = restored->command("CLIENT INFO");
    restored.reset();
    return newId > 0 && newId != originalId && !info->isError() &&
        info->toString().find(" db=1 ") != std::string::npos && pool.totalCount() == 1 && pool.availableCount() == 1 ? 0 : 1;
}

/** 用实际 AUTH 失败检查预建失败后的可重试性和延迟建连失败后的名额归还
@return 池计数、两次独立认证错误和恢复后的会话均符合预期时返回 0
*/
static int RunPoolAuthFailure()
{
    HiRedisConfig valid;
    if (!ReadConfig(valid) || valid.m_password.empty())
    {
        return 1;
    }
    HiRedisConfig invalid = valid;
    invalid.setPassword(valid.m_password + "_invalid");
    invalid.setPoolBounds(1, 1);
    HiRedisConnectionPool pool;
    if (pool.init(invalid) || pool.lastError().find("WRONGPASS") == std::string::npos ||
        pool.totalCount() != 0 || pool.availableCount() != 0)
    {
        std::cerr << "Prewarming with invalid credentials did not report the authentication failure" << std::endl;
        return 1;
    }
    invalid.setPoolBounds(0, 1);
    if (!pool.init(invalid) || !pool.lastError().empty())
    {
        return 1;
    }
    for (int attempt = 0; attempt < 2; ++attempt)
    {
        if (pool.acquire(0) || pool.lastError().find("WRONGPASS") == std::string::npos ||
            pool.totalCount() != 0 || pool.availableCount() != 0)
        {
            std::cerr << "Failed lazy authentication did not release the only pool slot" << std::endl;
            return 1;
        }
    }
    pool.close();
    valid.setPoolBounds(0, 1);
    if (!pool.init(valid))
    {
        return 1;
    }
    auto loan = pool.acquire(0);
    if (!loan || !loan->ping() || pool.totalCount() != 1 || pool.availableCount() != 0 || !pool.lastError().empty())
    {
        return 1;
    }
    loan.reset();
    return pool.totalCount() == 0 && pool.availableCount() == 0 ? 0 : 1;
}

/** 并发核对有限池的名额、空闲保留、外借副本以及关闭重开
@return 八个借用尝试的会话身份和各阶段计数正确时返回 0
*/
static int RunConcurrentCapacity()
{
    HiRedisConfig config;
    if (!ReadConfig(config))
    {
        return 1;
    }
    config.setPoolBounds(1, 3);
    HiRedisConnectionPool pool;
    if (!pool.init(config) || pool.availableCount() != 1)
    {
        return 1;
    }
    std::promise<void> start;
    auto gate = start.get_future().share();
    std::vector<std::future<std::shared_ptr<HiRedis>>> attempts;
    attempts.reserve(8);
    try
    {
        for (size_t i = 0; i < 8; ++i)
        {
            attempts.push_back(std::async(std::launch::async, [gate, &pool]() {
                gate.wait();
                return pool.acquire(0);
            }));
        }
    }
    catch (const std::exception& error)
    {
        start.set_value();
        pool.close();
        std::cerr << error.what() << std::endl;
        return 1;
    }
    start.set_value();
    std::vector<std::shared_ptr<HiRedis>> loans;
    std::vector<int64_t> ids;
    for (auto& attempt : attempts)
    {
        if (attempt.wait_for(std::chrono::seconds(5)) != std::future_status::ready)
        {
            pool.close();
            return 1;
        }
        auto loan = attempt.get();
        if (loan)
        {
            ids.push_back(loan->command("CLIENT ID")->toInt64());
            loans.push_back(loan);
        }
    }
    std::sort(ids.begin(), ids.end());
    if (ids.size() != 3 || ids.front() <= 0 || std::adjacent_find(ids.begin(), ids.end()) != ids.end() ||
        pool.totalCount() != 3 || pool.availableCount() != 0)
    {
        std::cerr << "Expected three distinct outstanding connections" << std::endl;
        return 1;
    }
    auto last = loans.back();
    loans.clear();
    if (pool.totalCount() != 2 || pool.availableCount() != 1)
    {
        return 1;
    }
    pool.close();
    config.setPoolBounds(0, 1);
    if (pool.totalCount() != 1 || pool.availableCount() != 0 || pool.init(config) || !last->ping())
    {
        return 1;
    }
    last.reset();
    if (pool.totalCount() != 0 || !pool.init(config))
    {
        return 1;
    }
    auto fresh = pool.acquire(0);
    const int64_t newId = fresh ? fresh->command("CLIENT ID")->toInt64() : 0;
    fresh.reset();
    return newId > 0 && !std::binary_search(ids.begin(), ids.end(), newId) &&
        pool.availableCount() == 0 && pool.totalCount() == 0 ? 0 : 1;
}

/** 模拟服务端拒绝事务结束命令，观察原会话及诊断
@param [in] denyExec 为 true 时拒绝 EXEC，否则拒绝入队出错后的 DISCARD
@return 原错误保留、原会话销毁且可以重连时返回 0
*/
static int RunTransactionRecovery(bool denyExec)
{
    HiRedisConfig config;
    redisContextFuncs hooks = {};
    HiRedisTransactionProbe observation = {nullptr, denyExec, false};
    HiRedis redis;
    if (!ReadConfig(config) || !redis.init(config))
    {
        return 1;
    }
    redisContext* context = redis.*ContextMember(HiRedisContextMember());
    if (!context || !context->funcs || !context->funcs->write || context->privdata || context->free_privdata)
    {
        return 1;
    }
    observation.m_transport = context->funcs;
    hooks = *context->funcs;
    // active 为本用例独占上下文，返回原传输的实际写入字节数或错误。
    hooks.write = [](redisContext* active) -> ssize_t
    {
        auto* probe = static_cast<HiRedisTransactionProbe*>(active->privdata);
        const char* expected = probe->m_denyExec ? "*1\r\n$4\r\nEXEC\r\n" : "*1\r\n$7\r\nDISCARD\r\n";
        if (std::strcmp(active->obuf, expected) == 0)
        {
            // 保持 RESP 长度，用同长未知命令取得真实服务端错误并保留未完成的 MULTI。
            const char* verb = probe->m_denyExec ? "EXEC" : "DISCARD";
            std::memset(std::strstr(active->obuf, verb), 'X', std::strlen(verb));
            probe->m_intercepted = true;
        }
        return probe->m_transport->write(active);
    };
    context->privdata = &observation;
    context->funcs = &hooks;
    const auto result = redis.execTransaction({HiRedisCommand({denyExec ? "PING" : "GET"})});
    const char* expectedError = denyExec ? "unknown command" : "wrong number of arguments";
    if (!observation.m_intercepted || !result->isError() || result->error().find(expectedError) == std::string::npos ||
        redis.lastError() != result->error() || redis.*ContextMember(HiRedisContextMember()) != nullptr)
    {
        std::cerr << (denyExec ? "EXEC" : "DISCARD") << " rejection left a session open or lost its error" << std::endl;
        return 1;
    }
    return redis.reconnect() && redis.ping() ? 0 : 1;
}

/** 核对无参借用在满池时使用配置快照中的无限等待或零等待
@return 两种期限和归还后的借用身份正确时返回 0
*/
static int RunDefaultWait()
{
    HiRedisConfig config;
    HiRedisConnectionPool pool;
    if (!ReadConfig(config))
    {
        return 1;
    }
    config.setPoolBounds(1, 1);
    config.setPoolAcquireTimeout(-1);
    if (!pool.init(config))
    {
        return 1;
    }
    auto held = pool.acquire();
    if (!held)
    {
        return 1;
    }
    const int64_t id = held->command("CLIENT ID")->toInt64();
    config.setPoolAcquireTimeout(0);
    std::promise<void> entered;
    auto started = entered.get_future();
    auto waiting = std::async(std::launch::async, [&]() {
        entered.set_value();
        return pool.acquire();
    });
    started.wait();
    const bool blocked = waiting.wait_for(std::chrono::milliseconds(3200)) == std::future_status::timeout;
    held.reset();
    if (waiting.wait_for(std::chrono::seconds(2)) != std::future_status::ready)
    {
        pool.close();
        return 1;
    }
    auto acquired = waiting.get();
    if (!blocked || !acquired || id <= 0 || acquired->command("CLIENT ID")->toInt64() != id)
    {
        return 1;
    }
    acquired.reset();
    pool.close();
    if (!pool.init(config) || !(held = pool.acquire()))
    {
        return 1;
    }
    auto immediate = std::async(std::launch::async, [&]() { return pool.acquire(); });
    if (immediate.wait_for(std::chrono::seconds(1)) != std::future_status::ready)
    {
        pool.close();
        return 1;
    }
    return !immediate.get() && pool.lastError() == "redis pool acquire timeout" ? 0 : 1;
}

/** 通过实际套接字、回复内容与缓冲容量验证连接初始化参数
@return 默认值、显式限制、无限模式及无效输入的行为均正确时返回 0
*/
static int RunReaderSettings()
{
    HiRedisConfig config;
    if (!ReadConfig(config) || config.m_keepAliveIdleSeconds != 15 || config.m_readerMaxUnusedBytes != 16384 ||
        config.m_readerMaxElements != INT64_C(4294967295))
    {
        return 1;
    }
    HiRedis redis;
    const std::string large(65536, 'v');
    // 首行保留构造默认值；其余四行分别控制缓冲回收和数组元素上限。
    const int64_t limits[][2] = {{16384, INT64_C(4294967295)}, {1024, 2}, {-1, -1}, {-1, 2}, {1024, -1}};
    const int idleSeconds[] = {15, 29, 41, 29, 41};
    for (size_t mode = 0; mode < sizeof(limits) / sizeof(limits[0]); ++mode)
    {
        const int64_t bufferLimit = limits[mode][0], elementLimit = limits[mode][1];
        if (mode != 0)
        {
            config.setReaderLimits(bufferLimit, elementLimit);
            config.setKeepAliveIdleSeconds(idleSeconds[mode]);
        }
        if (!redis.init(config))
        {
            std::cerr << redis.lastError() << std::endl;
            return 1;
        }
        const auto* context = redis.*ContextMember(HiRedisContextMember());
        int idle = 0;
#ifdef _WIN32
        int length = sizeof(idle);
        const int idleOption = TCP_KEEPALIVE;
#else
        socklen_t length = sizeof(idle);
        const int idleOption = TCP_KEEPIDLE;
#endif
        if (!context || !context->reader ||
            getsockopt(context->fd, IPPROTO_TCP, idleOption, reinterpret_cast<char*>(&idle), &length) != 0 ||
            idle != idleSeconds[mode] ||
            context->reader->maxbuf != static_cast<size_t>(bufferLimit < 0 ? 0 : bufferLimit) ||
            context->reader->maxelements != (elementLimit < 0 ? 0 : elementLimit))
        {
            std::cerr << "Socket or reader configuration mismatch in mode " << mode << std::endl;
            return 1;
        }
        if (redis.command(std::vector<std::string>{"ECHO", large})->toString() != large || !redis.ping() ||
            (sdsavail(context->reader->buf) >= large.size() / 2) != (bufferLimit == -1))
        {
            std::cerr << "Reader did not apply the unused buffer threshold in mode " << mode << std::endl;
            return 1;
        }
        const auto pair = redis.command(std::vector<std::string>{"EVAL", "return {1,2}", "0"});
        const auto triple = redis.command(std::vector<std::string>{"EVAL", "return {1,2,3}", "0"});
        if (pair->toKeys() != std::vector<std::string>({"1", "2"}) || (elementLimit == 2 ?
            (!triple->isError() || triple->error().find("Multi-bulk length out of range") == std::string::npos || redis.isConnected() || !redis.reconnect() || !redis.ping()) :
            triple->toKeys() != std::vector<std::string>({"1", "2", "3"})))
        {
            std::cerr << "Reader did not enforce the aggregate element limit in mode " << mode << std::endl;
            return 1;
        }
    }
    const int64_t id = redis.command("CLIENT ID")->toInt64();
    config.setPoolBounds(0, 1);
    for (int i = 0; i < 6; ++i)
    {
        HiRedisConfig invalid = config;
        if (i < 2)
        {
            invalid.setReaderLimits(i == 0 ? 0 : -2, -1);
        }
        else if (i < 4)
        {
            invalid.setReaderLimits(-1, i == 2 ? 0 : -2);
        }
        else
        {
            invalid.setKeepAliveIdleSeconds(i == 4 ? 0 : INT32_MAX);
        }
        HiRedisConnectionPool lazy;
        if (redis.init(invalid) || redis.lastError().empty() || lazy.init(invalid) || lazy.lastError().empty() ||
            id <= 0 || redis.command("CLIENT ID")->toInt64() != id)
        {
            return 1;
        }
    }
    if (sizeof(size_t) == 4)
    {
        config.setReaderLimits(INT64_C(4294967296), -1);
        if (redis.init(config) || redis.command("CLIENT ID")->toInt64() != id)
        {
            return 1;
        }
    }
    return 0;
}

/** 在独立进程中核对离线接口或连接到真实的隔离服务。
@param [in] online 为 true 时执行真实数据库场景，否则执行离线检查
@param [in] benchmark 连接成功后执行数据库计时
@return 成功返回 0，断言失败返回 1
*/
static int RunChecks(bool online, bool benchmark)
{
    int32_t totalCount = 0;
    int32_t failCount = 0;
    int32_t skipCount = 0;
    std::function<void(bool, const std::string&)> check = [&totalCount, &failCount](bool ok, const std::string& name) -> void
    {
        ++totalCount;
        std::cout << (ok ? "[PASS] " : "[FAIL] ") << name << std::endl;
        if (!ok)
        {
            ++failCount;
        }
    };
    std::function<void(const std::string&)> skip = [&skipCount](const std::string& name) -> void
    {
        ++skipCount;
        std::cout << "[SKIP] " << name << std::endl;
    };
    std::function<int32_t()> currentProcessId = []() -> int32_t
    {
#ifdef _WIN32
        return static_cast<int32_t>(_getpid());
#else
        return static_cast<int32_t>(getpid());
#endif
    };

    if (!online)
    {
        HiRedisConfig defaults;
        check(defaults.m_ip == "127.0.0.1" && defaults.m_port == 6379 && defaults.m_password.empty() &&
            defaults.m_dbIndex == 0 && defaults.m_connectTimeoutMs == 3000 && defaults.m_commandTimeoutMs == 3000 &&
            defaults.m_enableKeepAlive && defaults.m_poolMinConnections == 1 && defaults.m_poolMaxConnections == 1 &&
            defaults.m_keepAliveIdleSeconds == 15 && defaults.m_readerMaxUnusedBytes == 16384 &&
            defaults.m_readerMaxElements == INT64_C(4294967295) && defaults.m_poolAcquireTimeoutMs == 3000, "default Redis configuration");
        std::vector<std::string> parsed;
        std::string error;
        check(HiRedisHelper::splitCommandText("SET 'two words' \"\" a\\ b", parsed, error) &&
            parsed == std::vector<std::string>({"SET", "two words", "", "a b"}) && error.empty(), "parse quoted and empty arguments");
        check(!HiRedisHelper::splitCommandText("GET '", parsed, error) && parsed.empty() && !error.empty(),
            "invalid text clears partial arguments");
        HiRedisConnectionPool lazy;
        defaults.setPoolBounds(0, -1);
        defaults.setEndpoint("", 6379);
        check(!lazy.init(defaults) && !lazy.lastError().empty(), "lazy pool rejects empty host before borrowing");
        defaults.setEndpoint("127.0.0.1", 65536);
        check(!lazy.init(defaults) && !lazy.lastError().empty(), "lazy pool rejects invalid TCP port");
        HiRedis disconnected;
        check(!disconnected.isConnected(), "HiRedis default disconnected");
        check(!disconnected.ping(), "HiRedis ping disconnected");
        std::shared_ptr<HiRedisResultSet> disconnectedResult = disconnected.command(std::vector<std::string>{"GET", "missing"});
        check(disconnectedResult && disconnectedResult->isError() && !disconnectedResult->isValid() &&
            disconnectedResult->isNull(), "disconnected command has a local error and no reply");
        std::vector<HiRedisCommand> disconnectedCommands;
        disconnectedCommands.push_back(HiRedisCommand(std::vector<std::string>{"GET", "missing"}));
        std::vector<std::shared_ptr<HiRedisResultSet>> disconnectedPipelineResult = disconnected.pipeline(disconnectedCommands);
        check(disconnectedPipelineResult.size() == 1 && disconnectedPipelineResult[0]->isError(), "HiRedis disconnected pipeline error");
        std::shared_ptr<HiRedisResultSet> disconnectedTransactionResult = disconnected.execTransaction(disconnectedCommands);
        check(disconnectedTransactionResult.get() != nullptr && disconnectedTransactionResult->isError(), "HiRedis disconnected transaction error");
        return failCount == 0 ? 0 : 1;
    }

    HiRedisConfig config;
    if (!ReadConfig(config))
    {
        return 1;
    }

    HiRedis redis;
    if (!redis.init(config))
    {
        check(false, "HiRedis test instance connection failed: " + redis.lastError());
        std::cout << "HiRedis test " << (failCount == 0 ? "PASS" : "FAIL") <<
            ", total=" << totalCount << ", failed=" << failCount << ", skipped=" << skipCount << std::endl;
        return 1;
    }
    const std::shared_ptr<HiRedisResultSet> server = redis.command(std::vector<std::string>{"INFO", "server"});
    if (!server || server->toString().find("redis_version:8.10.3\r\n") == std::string::npos)
    {
        check(false, "HiRedis requires Redis server 8.10.3");
        return 1;
    }
    check(server && server->toString().find("redis_version:8.10.3\r\n") != std::string::npos, "HiRedis Redis server 8.10.3");
    if (benchmark)
    {
        try
        {
            RunDatabaseBenchmark(redis);
        }
        catch (const std::exception& error)
        {
            std::cerr << error.what() << std::endl;
            return 1;
        }
        return failCount == 0 ? 0 : 1;
    }
    const std::string echoBytes("a\0b", 3);
    std::shared_ptr<HiRedisResultSet> retained;
    int64_t scopedId = 0;
    {
        HiRedis scopedRedis;
        check(scopedRedis.init(config), "HiRedis destructor scoped init");
        retained = scopedRedis.command(HiRedisCommand({"ECHO", echoBytes}));
        scopedId = scopedRedis.command("CLIENT ID")->toInt64();
        check(ClientState(redis, scopedId, true), "scoped client is visible before destruction");
    }
    check(ClientState(redis, scopedId, false) && redis.ping(), "destructor closed the observed client");
    const redisReply* raw = retained ? retained->toReply() : nullptr;
    check(raw && raw->type == REDIS_REPLY_STRING && raw->len == echoBytes.size() && raw->str &&
        std::memcmp(raw->str, echoBytes.data(), echoBytes.size()) == 0 && retained->toString() == echoBytes,
        "single HiRedisCommand preserves binary data and the result owns it after client destruction");
    {
        HiRedis keepAliveOff;
        HiRedisConfig options = config;
        options.m_enableKeepAlive = false;
        check(keepAliveOff.init(options) && keepAliveOff.ping(), "Redis connects without TCP keepalive");
        const auto high = redis.command(std::vector<std::string>{"EVAL", "return 2147483664", "0"});
        const auto low = redis.command(std::vector<std::string>{"EVAL", "return -2147483664", "0"});
        check(high->toInt64() == INT64_C(2147483664) && high->toCount() == INT32_MAX,
            "32-bit count saturates above its maximum");
        check(low->toInt64() == -INT64_C(2147483664) && low->toCount() == INT32_MIN,
            "32-bit count saturates below its minimum");
    }

    std::ostringstream prefixStream;
    prefixStream << "hiredis_test_" << static_cast<int64_t>(std::time(nullptr)) << "_" <<
        currentProcessId() << "_" << reinterpret_cast<uintptr_t>(&prefixStream) << "_";
    const std::string prefix = prefixStream.str();
    const std::string stringKey = prefix + "string";
    const std::string binaryKey = prefix + "binary";
    const std::string rawPercentKey = prefix + "raw_percent";
    const std::string rawEscapeKey = prefix + "raw_escape";
    const std::string rawTrailingEscapeKey = prefix + "raw_trailing_escape";
    const std::string nxKey = prefix + "nx";
    const std::string intKey = prefix + "int";
    const std::string mapKey1 = prefix + "map_1";
    const std::string mapKey2 = prefix + "map_2";
    const std::string deleteOneKey = prefix + "delete_one";
    const std::string deleteManyKey1 = prefix + "delete_many_1";
    const std::string deleteManyKey2 = prefix + "delete_many_2";
    const std::string hashKey = prefix + "hash";
    const std::string tableKey = prefix + "table";
    const std::string groupKey = prefix + "group";
    const std::string orderKey = prefix + "order";
    const std::string transactionKey = prefix + "transaction";

    check(redis.isConnected(), "HiRedis connected");
    check(redis.ping(), "HiRedis ping connected");
    const int32_t testDatabase = config.m_dbIndex < 0 ? 0 : config.m_dbIndex;
    check(redis.selectDbIndex(testDatabase), "HiRedis selectDbIndex");
    check(redis.selectDb(testDatabase), "HiRedis selectDb");
    HiRedis endpointClient;
    // 两个简写入口默认不带密码，认证实例必须在隐式 SELECT 时返回 NOAUTH。
    const bool endpointInit = endpointClient.init(config.m_ip, config.m_port);
    check(config.m_password.empty() ? endpointInit && endpointClient.ping() :
        !endpointInit && !endpointClient.isConnected() && endpointClient.lastError().find("NOAUTH") != std::string::npos,
        "init(host, port) uses the requested endpoint and default credentials");
    const bool endpointConnect = endpointClient.connect(config.m_ip, config.m_port);
    check(config.m_password.empty() ? endpointConnect && endpointClient.ping() :
        !endpointConnect && !endpointClient.isConnected() && endpointClient.lastError().find("NOAUTH") != std::string::npos,
        "connect(host, port) uses the requested endpoint and default credentials");
    check(endpointClient.connect(config), "HiRedis connect(config)");
    const int64_t originalClient = endpointClient.command(std::vector<std::string>{"CLIENT", "ID"})->toInt64();
    HiRedisConfig invalidHost = config;
    invalidHost.setEndpoint(config.m_ip + std::string("\0suffix", 7), config.m_port);
    const bool hostRejected = !endpointClient.connect(invalidHost) && !endpointClient.lastError().empty();
    const int64_t currentClient = endpointClient.command(std::vector<std::string>{"CLIENT", "ID"})->toInt64();
    check(hostRejected && originalClient > 0 && originalClient == currentClient,
        "reject embedded NUL hostname before replacing active connection");
    check(endpointClient.ping(), "HiRedis connect(config) ping");
    endpointClient.close();
    check(!endpointClient.isConnected(), "HiRedis close");
    if (!config.m_password.empty())
    {
        check(redis.auth(config.m_password), "explicit AUTH accepts configured password");
    }
    check(!redis.auth(config.m_password + "_invalid") && !redis.lastError().empty(),
        "explicit AUTH rejects an incorrect password");
    check(redis.reconnect() && redis.isConnected() && redis.ping(), "HiRedis reconnect");
    check(RejectsUnavailableEndpoint(redis), "unavailable local TCP endpoint fails to connect");
    HiRedisConfig invalidDbConfig = config;
    invalidDbConfig.m_dbIndex = 100000;
    HiRedis invalidDbRedis;
    check(!invalidDbRedis.init(invalidDbConfig) && !invalidDbRedis.isConnected(), "HiRedis init invalid db failed");
    HiRedisConfig invalidAuthConfig = config;
    invalidAuthConfig.setPassword(config.m_password + std::string("\0bad", 4));
    HiRedis invalidAuthRedis;
    check(!invalidAuthRedis.init(invalidAuthConfig) && !invalidAuthRedis.isConnected(), "HiRedis init invalid auth failed");
    check(redis.set(stringKey, "value"), "HiRedis set");
    check(!redis.orderGroupElementExist(stringKey, "x") && !redis.lastError().empty(),
        "wrong key type never reports sorted set membership");
    check(redis.getOrderGroupElementIndex(stringKey, "x") == -1 && !redis.lastError().empty(),
        "wrong key type has no rank");
    check(redis.getOrderGroupElementScore(stringKey, "x") == -1 && !redis.lastError().empty(),
        "wrong key type has no score");
    check(redis.get(stringKey)->toString() == "value", "HiRedis get");
    check(redis.setKeyValue(stringKey, "value2"), "HiRedis setKeyValue");
    check(redis.getValue(stringKey)->toString() == "value2", "HiRedis getValue");
    std::string binaryValue("a\0b", 3);
    check(redis.set(binaryKey, binaryValue), "HiRedis set binary value");
    std::string binaryRead = redis.get(binaryKey)->toString();
    check(binaryRead.size() == 3 && binaryRead[0] == 'a' && binaryRead[1] == '\0' && binaryRead[2] == 'b', "HiRedis get binary value");
    check(redis.command(std::string("SET ") + rawPercentKey + " \"%hello world\"")->toStatus(), "HiRedis raw command quote percent");
    check(redis.get(rawPercentKey)->toString() == "%hello world", "HiRedis raw command quote result");
    check(redis.command(std::string("SET ") + rawEscapeKey + " a\\ b")->toStatus(), "HiRedis raw command escape space");
    check(redis.get(rawEscapeKey)->toString() == "a b", "HiRedis raw command escape result");
    check(redis.command(std::string("SET ") + rawTrailingEscapeKey + " abc\\")->toStatus(), "HiRedis raw command trailing escape");
    check(redis.get(rawTrailingEscapeKey)->toString() == "abc\\", "HiRedis raw command trailing escape result");
    const auto emptyString = redis.command(std::vector<std::string>{"ECHO", ""});
    check(emptyString->isValid() && !emptyString->isNull() && emptyString->toString().empty(),
        "empty bulk string is distinct from a missing reply");
    check(redis.command("")->isError(), "HiRedis empty raw command error");
    check(redis.command("GET \"bad")->isError(), "HiRedis unclosed quote command error");
    check(redis.command(std::vector<std::string>())->isError(), "HiRedis empty argv command error");
    check(redis.setEx(prefix + "expire", "1", 30), "HiRedis setEx");
    const int64_t ttl = redis.ttl(prefix + "expire");
    check(ttl > 0 && ttl <= 30 && redis.get(prefix + "expire")->toString() == "1", "HiRedis ttl and setEx value");
    check(redis.setNx(nxKey, "first"), "HiRedis setNx first");
    check(!redis.setNx(nxKey, "second") && redis.get(nxKey)->toString() == "first" && redis.ttl(nxKey) == -1,
        "SET NX preserves the original value without expiry");
    check(redis.deleteKey(nxKey) && redis.ttl(nxKey) == -2 && redis.setNx(nxKey, "expiring", 60),
        "SET NX recreates a missing key with expiry");
    const int64_t nxTtl = redis.ttl(nxKey);
    check(nxTtl > 0 && nxTtl <= 60 && redis.get(nxKey)->toString() == "expiring", "SET NX applied the requested TTL");
    check(redis.incrBy(intKey, 10) == 10, "HiRedis incrBy");
    check(redis.decrBy(intKey, 3) == 7, "HiRedis decrBy");
    check(redis.expire(intKey, 30) && redis.ttl(intKey) > 0 && !redis.expire(prefix + "missing", 30), "HiRedis expire");
    check(redis.keyExist(intKey), "HiRedis keyExist");
    check(redis.del(std::vector<std::string>()) == 0, "HiRedis del empty");
    check(redis.set(deleteOneKey, "1") && redis.deleteKey(deleteOneKey) && !redis.keyExist(deleteOneKey), "HiRedis deleteKey");
    check(redis.set(deleteManyKey1, "1") && redis.set(deleteManyKey2, "2") &&
        redis.deleteKeys(std::vector<std::string>{deleteManyKey1, deleteManyKey2}) &&
        !redis.keyExist(deleteManyKey1) && !redis.keyExist(deleteManyKey2), "HiRedis deleteKeys");

    std::map<std::string, std::string> valueMap;
    valueMap[mapKey1] = "m1";
    valueMap[mapKey2] = "m2";
    check(redis.setMap(valueMap), "HiRedis setMap");
    check(!redis.setMap(std::map<std::string, std::string>()), "HiRedis setMap empty false");
    std::map<std::string, std::string> getMap = redis.mGet(std::vector<std::string>{mapKey1, mapKey2})->toMap();
    check(getMap[mapKey1] == "m1" && getMap[mapKey2] == "m2", "HiRedis mGet toMap");
    std::map<std::string, std::string> getValuesMap = redis.getValues(std::vector<std::string>{mapKey1, mapKey2})->toMap();
    check(getValuesMap[mapKey1] == "m1" && getValuesMap[mapKey2] == "m2", "HiRedis getValues");
    std::map<std::string, std::string> missingMap = redis.mGet(std::vector<std::string>{prefix + "missing", mapKey1})->toMap();
    check(missingMap.find(prefix + "missing") == missingMap.end() && missingMap[mapKey1] == "m1", "HiRedis mGet missing nil skip");

    check(redis.command("MULTI")->toStatus(), "begin raw transaction before queued integer reply");
    check(!redis.hSet(hashKey, "pending", "value"), "queued hash write has no integer completion reply");
    const auto cancelledWrite = redis.command("DISCARD");
    const auto absentField = redis.hGet(hashKey, "pending");
    check(cancelledWrite->toStatus() && absentField->isValid() && absentField->isNull(),
        "discarded hash write has no observable effect");
    check(redis.hSet(hashKey, "field1", "value1"), "HiRedis hSet");
    std::map<std::string, std::string> hashMap;
    hashMap["field2"] = "value2";
    hashMap["field3"] = "value3";
    check(redis.hmSet(hashKey, hashMap), "HiRedis hmSet");
    check(redis.hGet(hashKey, "field2")->toString() == "value2", "hmSet stores the original field");
    hashMap["field2"] = "replacement";
    check(redis.setHashMap(hashKey, hashMap), "HiRedis setHashMap");
    check(!redis.hmSet(hashKey, std::map<std::string, std::string>()), "HiRedis hmSet empty false");
    check(redis.hGet(hashKey, "field1")->toString() == "value1", "HiRedis hGet");
    std::map<std::string, std::string> readHash = redis.hGetAll(hashKey)->toHashMap();
    check(readHash["field2"] == "replacement" && readHash["field3"] == "value3", "HiRedis hGetAll");
    check(redis.hmGet(hashKey, std::vector<std::string>{"field1", "field2"})->toMap()["field1"] == "value1", "HiRedis hmGet");
    check(redis.getHashValues(hashKey)->toHashMap()["field1"] == "value1", "HiRedis getHashValues all");
    check(redis.getHashValues(hashKey, std::vector<std::string>{"field1"})->toMap()["field1"] == "value1", "HiRedis getHashValues fields");
    auto fields = redis.hKeys(hashKey)->toKeys();
    std::sort(fields.begin(), fields.end());
    check(fields == std::vector<std::string>({"field1", "field2", "field3"}), "HiRedis hKeys");
    auto aliasFields = redis.getHashKeys(hashKey)->toKeys();
    std::sort(aliasFields.begin(), aliasFields.end());
    check(aliasFields == fields, "HiRedis getHashKeys");
    check(redis.hashMapExist(hashKey), "HiRedis hashMapExist");
    check(!redis.hDel(hashKey, std::vector<std::string>()), "HiRedis hDel empty false");
    check(redis.deleteHashMapField(hashKey, "field3"), "HiRedis deleteHashMapField");
    check(redis.deleteHashMapFields(hashKey, std::vector<std::string>{"field2"}), "HiRedis deleteHashMapFields");
    check(redis.hGetAll(hashKey)->toMap() == std::map<std::string, std::string>({{"field1", "value1"}}),
        "hash deletion removed exactly the requested fields");
    check(redis.deleteHashMap(hashKey) && !redis.hashMapExist(hashKey), "HiRedis deleteHashMap");

    std::vector<std::vector<std::string>> table;
    table.push_back(std::vector<std::string>{"row1", "name1", "score1"});
    table.push_back(std::vector<std::string>{"row2", "name2", "score2"});
    check(redis.setTable(tableKey, table, 0, "|"), "HiRedis setTable");
    check(!redis.setTable(prefix + "empty_table", std::vector<std::vector<std::string>>(), 0, "|"), "HiRedis setTable empty false");
    check(redis.tableExist(tableKey), "HiRedis tableExist");
    std::vector<std::vector<std::string>> tableRead = redis.getTableValues(tableKey)->toTable(0, "|");
    std::sort(tableRead.begin(), tableRead.end());
    check(tableRead == table, "HiRedis getTableValues");
    std::vector<std::vector<std::string>> tableMissingRead = redis.getTableValues(tableKey, std::vector<std::string>{prefix + "missing", "row1"})->toTable(0, "|");
    check(tableMissingRead.size() == 1 && tableMissingRead[0] == table[0], "HiRedis getTableValues missing nil skip");
    const std::vector<std::vector<std::string>> emptyColumns = {
        {"", "row3", "", "tail", ""}, {"", "row4", "", "", ""}
    };
    check(redis.setTable(tableKey, emptyColumns, 1, "||"), "HiRedis setTable accepts empty cells");
    check(redis.getTableValues(tableKey, {"row3", "row4"})->toTable(1, "||") == emptyColumns,
        "HiRedis table preserves leading repeated and trailing empty cells");

    check(redis.setGroup(groupKey, "a"), "HiRedis setGroup");
    check(redis.setGroups(groupKey, std::vector<std::string>{"b", "c"}), "HiRedis setGroups");
    check(!redis.setGroups(groupKey, std::vector<std::string>()), "HiRedis setGroups empty false");
    check(redis.groupExist(groupKey), "HiRedis groupExist");
    check(redis.groupElementExist(groupKey, "b"), "HiRedis groupElementExist");
    check(redis.getGroupCount(groupKey) == 3, "HiRedis getGroupCount");
    auto members = redis.getGroup(groupKey)->toGroup();
    std::sort(members.begin(), members.end());
    check(members == std::vector<std::string>({"a", "b", "c"}), "HiRedis getGroup");
    check(redis.deleteGroupElement(groupKey, "c"), "HiRedis deleteGroupElement");
    check(!redis.deleteGroupElements(groupKey, std::vector<std::string>()), "HiRedis deleteGroupElements empty false");
    check(redis.setGroups(groupKey, std::vector<std::string>{"d", "e"}), "HiRedis setGroups for deleteGroupElements");
    check(redis.deleteGroupElements(groupKey, std::vector<std::string>{"d", "e"}), "HiRedis deleteGroupElements");

    members = redis.getGroup(groupKey)->toGroup();
    std::sort(members.begin(), members.end());
    check(members == std::vector<std::string>({"a", "b"}), "set deletion preserves exactly the remaining members");

    check(redis.setOrderGroup(orderKey, 10, "oa"), "HiRedis setOrderGroup");
    check(redis.setOrderGroups(orderKey, std::vector<std::pair<int32_t, std::string>>{std::make_pair(20, "ob"), std::make_pair(30, "oc")}), "HiRedis setOrderGroups");
    check(!redis.setOrderGroups(orderKey, std::vector<std::pair<int32_t, std::string>>()), "HiRedis setOrderGroups empty false");
    check(redis.orderGroupExist(orderKey), "HiRedis orderGroupExist");
    check(redis.orderGroupElementExist(orderKey, "ob"), "HiRedis orderGroupElementExist");
    check(redis.getOrderGroupElementIndex(orderKey, "ob") == 1, "HiRedis getOrderGroupElementIndex");
    check(redis.getOrderGroupElementScore(orderKey, "ob") == 20, "HiRedis getOrderGroupElementScore");
    check(redis.getOrderGroupCount(orderKey) == 3, "HiRedis getOrderGroupCount");
    check(redis.getOrderGroup(orderKey)->toKeys() == std::vector<std::string>({"oa", "ob", "oc"}), "HiRedis getOrderGroup");
    check(redis.getOrderGroupByIndex(orderKey, 0, -1)->toKeys() == std::vector<std::string>({"oa", "ob", "oc"}), "HiRedis getOrderGroupByIndex");
    check(redis.getOrderGroupByScore(orderKey, 10, 30)->toKeys() == std::vector<std::string>({"oa", "ob", "oc"}), "HiRedis getOrderGroupByScore");
    check(redis.getOrderGroupCountByScore(orderKey, 10, 30) == 3, "HiRedis getOrderGroupCountByScore");
    check(redis.deleteOrderGroupElement(orderKey, "oc"), "HiRedis deleteOrderGroupElement");
    check(!redis.deleteOrderGroupElements(orderKey, std::vector<std::string>()), "HiRedis deleteOrderGroupElements empty false");
    check(redis.setOrderGroups(orderKey, std::vector<std::pair<int32_t, std::string>>{std::make_pair(40, "od"), std::make_pair(50, "oe"), std::make_pair(60, "of")}), "HiRedis setOrderGroups for range delete");
    check(redis.deleteOrderGroupElements(orderKey, std::vector<std::string>{"od"}), "HiRedis deleteOrderGroupElements");
    check(redis.deleteOrderGroupElementsByScore(orderKey, 50, 50) == 1, "HiRedis deleteOrderGroupElementsByScore");
    check(redis.deleteOrderGroupElementsByIndex(orderKey, 0, 0) == 1, "HiRedis deleteOrderGroupElementsByIndex");

    check(redis.getOrderGroup(orderKey)->toKeys() == std::vector<std::string>({"ob", "of"}),
        "rank, score and member deletion leave the expected ordered members");

    std::vector<HiRedisCommand> pipelineCommands;
    pipelineCommands.push_back(HiRedisCommand(std::vector<std::string>{"SET", prefix + "pipeline", "pv"}));
    pipelineCommands.push_back(HiRedisCommand(std::vector<std::string>{"GET", prefix + "pipeline"}));
    std::vector<std::shared_ptr<HiRedisResultSet>> pipelineResult = redis.pipeline(pipelineCommands);
    check(pipelineResult.size() == 2 && pipelineResult[0]->toStatus() && pipelineResult[1]->toString() == "pv", "HiRedis pipeline");
    check(redis.pipeline(std::vector<HiRedisCommand>()).empty(), "HiRedis pipeline empty");
    std::vector<HiRedisCommand> invalidPipelineCommands;
    invalidPipelineCommands.push_back(HiRedisCommand());
    std::vector<std::shared_ptr<HiRedisResultSet>> invalidPipelineResult = redis.pipeline(invalidPipelineCommands);
    check(invalidPipelineResult.size() == 1 && invalidPipelineResult[0]->isError() && redis.ping(), "HiRedis pipeline invalid command safe");

    const auto invalidTail = redis.pipeline({HiRedisCommand({"SET", prefix + "pipeline", "partial"}), HiRedisCommand()});
    const std::string validationError = redis.lastError();
    const auto unchanged = redis.get(prefix + "pipeline");
    check(invalidTail.size() == 1 && invalidTail[0]->isError() && invalidTail[0]->error() == validationError &&
        unchanged->isValid() && unchanged->toString() == "pv", "pipeline validates every command before sending writes");

    const HiRedisCommand invalidGet(std::vector<std::string>{"GET"});
    const HiRedisCommand pingCommand(std::vector<std::string>{"PING"});
    const auto mixed = redis.pipeline({invalidGet, pingCommand});
    check(mixed.size() == 2 && mixed[0]->isError() && mixed[1]->toString() == "PONG" &&
        redis.lastError() == mixed[0]->error(), "pipeline drains replies and reports server command error");
    const auto cleanBatch = redis.pipeline({pingCommand});
    check(cleanBatch.size() == 1 && cleanBatch[0]->toString() == "PONG" && redis.lastError().empty(),
        "successful pipeline clears earlier batch error");
    HiRedis interrupted;
    check(interrupted.init(config), "separate session opened for interrupted pipeline");
    const auto interruptedBatch = interrupted.pipeline({HiRedisCommand({"QUIT"}), pingCommand});
    check(interruptedBatch.size() == 2 && interruptedBatch[0]->toString() == "OK" &&
        interruptedBatch[1]->isError() && !interrupted.isConnected() && interrupted.reconnect() && interrupted.ping(),
        "pipeline preserves completed reply, closes on transport failure and can reconnect");
    const auto rejected = redis.execTransaction({pingCommand, invalidGet});
    check(rejected->isError() && redis.lastError() == rejected->error() && redis.ping(),
        "transaction preserves queue error after discard and releases session");

    std::vector<HiRedisCommand> transactionCommands;
    transactionCommands.push_back(HiRedisCommand(std::vector<std::string>{"SET", transactionKey, "tv"}));
    transactionCommands.push_back(HiRedisCommand(std::vector<std::string>{"GET", transactionKey}));
    std::shared_ptr<HiRedisResultSet> transactionResult = redis.execTransaction(transactionCommands);
    check(transactionResult && transactionResult->isValid() && transactionResult->size() == 2 &&
        transactionResult->at(0) == "OK" && transactionResult->at(1) == "tv" &&
        redis.get(transactionKey)->toString() == "tv", "HiRedis execTransaction executes every queued command");
    const auto partial = redis.execTransaction({HiRedisCommand({"HGET", transactionKey, "field"}),
        HiRedisCommand({"ECHO", "WRONGTYPE"}), HiRedisCommand({"GET", transactionKey})});
    check(partial && !partial->isError() && partial->size() == 3 && partial->isErrorAt(0) &&
        partial->at(0).find("WRONGTYPE") == 0 && !partial->isErrorAt(1) && partial->at(1) == "WRONGTYPE" &&
        !partial->isErrorAt(2) && partial->at(2) == "tv" && !partial->isErrorAt(3) &&
        !redis.get(transactionKey)->isErrorAt(0), "EXEC reports individual errors and continues later commands");
    {
        HiRedis writer;
        const int64_t sessionId = redis.command("CLIENT ID")->toInt64();
        check(writer.init(config) && redis.command(std::vector<std::string>{"WATCH", transactionKey})->toStatus() &&
            writer.set(transactionKey, "other writer"), "another session modified the watched key");
        const auto cancelled = redis.execTransaction({HiRedisCommand({"SET", transactionKey, "uncommitted"})});
        check(cancelled->isValid() && cancelled->isNull() && redis.get(transactionKey)->toString() == "other writer" &&
            sessionId > 0 && redis.command("CLIENT ID")->toInt64() == sessionId && redis.ping(),
            "WATCH conflict leaves the committed value and the original session intact");
        check(redis.set(transactionKey, "tv"), "restore transaction fixture");
    }
    const auto nested = redis.execTransaction({HiRedisCommand({"SET", transactionKey, "changed"}), HiRedisCommand({"eXeC"})});
    check(nested->isError() && redis.lastError() == nested->error() && redis.get(transactionKey)->toString() == "tv",
        "transaction rejects nested EXEC before any write");
    bool localeReady = true;
#ifdef _WIN32
    const std::string savedCType = std::setlocale(LC_CTYPE, nullptr);
    localeReady = std::setlocale(LC_CTYPE, "tr-TR") != nullptr;
#endif
    const auto discard = redis.execTransaction({HiRedisCommand({"SET", transactionKey, "changed"}), HiRedisCommand({"discard"})});
#ifdef _WIN32
    localeReady = std::setlocale(LC_CTYPE, savedCType.c_str()) != nullptr && localeReady;
#endif
    check(localeReady && discard->isError() && discard->error() == "redis transaction cannot contain session control commands" &&
        redis.get(transactionKey)->toString() == "tv", "Redis command names are ASCII independent of process locale");
    std::shared_ptr<HiRedisResultSet> invalidTransactionResult = redis.execTransaction(invalidPipelineCommands);
    check(invalidTransactionResult.get() != nullptr && invalidTransactionResult->isError() && redis.ping(), "HiRedis execTransaction invalid command safe");

    HiRedisConnectionPool emptyPool;
    check(!emptyPool.init(config, 0), "HiRedisConnectionPool init zero false");
    check(emptyPool.acquire(1).get() == nullptr, "HiRedisConnectionPool acquire before init false");
    emptyPool.uninit();
    emptyPool.uninit();
    check(emptyPool.totalCount() == 0 && emptyPool.availableCount() == 0, "HiRedisConnectionPool duplicate uninit without init safe");
    {
        HiRedisConfig limits = config;
        HiRedisConnectionPool elastic;
        check(limits.m_poolMinConnections == 1 && limits.m_poolMaxConnections == 1, "pool default bounds");
        limits.m_poolMinConnections = -1;
        check(!elastic.init(limits), "pool negative minimum");
        limits.m_poolMinConnections = 0;
        limits.m_poolMaxConnections = -2;
        check(!elastic.init(limits), "pool invalid negative maximum");
        limits.m_poolMaxConnections = 0;
        check(!elastic.init(limits), "pool zero maximum");
        limits.m_poolMinConnections = 3;
        limits.m_poolMaxConnections = 2;
        check(!elastic.init(limits), "pool maximum below minimum");
        limits.m_poolMinConnections = 0;
        check(elastic.init(limits) && elastic.totalCount() == 0, "pool lazy initialization");
        auto first = elastic.acquire(0);
        auto second = elastic.acquire(0);
        check(first && second && first != second && elastic.totalCount() == 2, "pool growth to limit");
        check(!elastic.acquire(0), "pool limit immediate timeout");
        first.reset();
        second.reset();
        check(elastic.totalCount() == 0 && elastic.availableCount() == 0, "pool zero idle retention");
        elastic.close();
        limits.m_poolMinConnections = 1;
        limits.m_poolMaxConnections = -1;
        check(elastic.init(limits) && elastic.availableCount() == 1, "pool unbounded warm start");
        first = elastic.acquire();
        second = elastic.acquire();
        auto third = elastic.acquire();
        check(first && second && third && elastic.totalCount() == 3, "pool unbounded growth");
        first.reset();
        second.reset();
        third.reset();
        check(elastic.totalCount() == 1 && elastic.availableCount() == 1, "pool retains configured minimum");
        limits.m_poolMaxConnections = 1;
        for (int i = 0; i < 8; ++i)
        {
            std::unique_ptr<HiRedisConnectionPool> owner(new HiRedisConnectionPool);
            auto opened = owner->init(limits);
            auto borrowed = owner->acquire(0);
            check(opened && borrowed && borrowed->ping(), "pool borrowed before racing destruction");
            std::atomic<bool> start(false);
            std::thread returning([&]() {
                while (!start.load())
                {
                    std::this_thread::yield();
                }
                borrowed.reset();
            });
            start.store(true);
            owner.reset();
            returning.join();
            check(!borrowed && !owner, "pool concurrent return and destruction");
        }
        for (int i = 0; i < 2; ++i)
        {
            limits.m_port = i == 0 ? 0 : 65536;
            HiRedis invalid;
            check(!invalid.connect(limits) && !invalid.lastError().empty(), "reject port outside TCP range");
        }
    }

    {
        HiRedisConnectionPool closing;
        check(closing.init(config, 1), "fixed pool starts a new generation");
        auto held = closing.acquire(0);
        std::promise<void> ready;
        auto entered = ready.get_future();
        auto pending = std::async(std::launch::async, [&]() {
            ready.set_value();
            return closing.acquire(-1);
        });
        entered.wait();
        check(held && pending.wait_for(std::chrono::milliseconds(20)) == std::future_status::timeout,
            "indefinite acquire waits while fixed pool is full");
        closing.close();
        held.reset();
        const bool reopened = closing.init(config, 1);
        if (pending.wait_for(std::chrono::seconds(2)) != std::future_status::ready)
        {
            check(false, "old borrower did not finish after close");
            closing.close();
        }
        check(!pending.get(), "closing wakes old generation without lending a session");
        check(reopened && closing.acquire(0) != nullptr,
            "new generation accepts new borrowers");
    }

    HiRedisConnectionPool pool;
    bool poolOk = pool.init(config, 8);
    check(poolOk, "HiRedisConnectionPool init");
    if (poolOk)
    {
        check(pool.init(config, 8), "HiRedisConnectionPool double init no-op");
        check(pool.totalCount() == 8 && pool.availableCount() == 8, "HiRedisConnectionPool double init keep old pool");
        std::shared_ptr<HiRedis> poolRedis = pool.acquire(1000);
        check(poolRedis.get() != nullptr && poolRedis->ping(), "HiRedisConnectionPool acquire");
        poolRedis.reset();
        check(pool.availableCount() == 8, "HiRedisConnectionPool release");

        HiRedisConnectionPool onePool;
        bool onePoolOk = onePool.init(config, 1);
        check(onePoolOk, "HiRedisConnectionPool single init");
        if (onePoolOk)
        {
            std::shared_ptr<HiRedis> holdConnection = onePool.acquire(1000);
            std::shared_ptr<HiRedis> timeoutConnection = onePool.acquire(1);
            check(holdConnection.get() != nullptr && timeoutConnection.get() == nullptr, "HiRedisConnectionPool acquire timeout");
            holdConnection.reset();
            onePool.uninit();
        }

        HiRedisConnectionPool borrowedPool;
        bool borrowedPoolOk = borrowedPool.init(config, 1);
        check(borrowedPoolOk, "HiRedisConnectionPool borrowed init");
        if (borrowedPoolOk)
        {
            std::shared_ptr<HiRedis> borrowedConnection = borrowedPool.acquire(1000);
            check(borrowedConnection.get() != nullptr && borrowedPool.init(config, 1), "HiRedisConnectionPool double init with borrowed no-op");
            check(borrowedPool.totalCount() == 1 && borrowedPool.availableCount() == 0, "HiRedisConnectionPool double init keep borrowed state");
            borrowedPool.uninit();
            borrowedPool.uninit();
            check(borrowedPool.totalCount() == 1 && borrowedPool.availableCount() == 0, "HiRedisConnectionPool double uninit kept borrowed state");
            borrowedConnection.reset();
            check(borrowedPool.totalCount() == 0 && borrowedPool.availableCount() == 0, "HiRedisConnectionPool borrowed release after uninit");
        }

        std::unique_ptr<HiRedisConnectionPool> destroyPool(new HiRedisConnectionPool);
        const bool destroyPoolOk = destroyPool->init(config, 1);
        check(destroyPoolOk, "HiRedisConnectionPool destroy with borrowed init");
        if (destroyPoolOk)
        {
            auto borrowedAfterDestroy = destroyPool->acquire(1000);
            const int64_t id = borrowedAfterDestroy ? borrowedAfterDestroy->command("CLIENT ID")->toInt64() : 0;
            check(ClientState(redis, id, true), "borrowed connection exists before pool destruction");
            auto sharedLease = borrowedAfterDestroy;
            destroyPool.reset();
            borrowedAfterDestroy.reset();
            check(sharedLease && sharedLease->ping() && ClientState(redis, id, true),
                "remaining shared lease keeps the same session alive after pool destruction");
            sharedLease.reset();
            check(ClientState(redis, id, false) && redis.ping(), "last lease closes the observed connection after pool destruction");
        }

        std::unique_ptr<HiRedisConnectionPool> idleDestroyPool(new HiRedisConnectionPool);
        const bool idleDestroyPoolOk = idleDestroyPool->init(config, 1);
        check(idleDestroyPoolOk, "HiRedisConnectionPool destroy with idle init");
        auto idleLease = idleDestroyPool->acquire(1000);
        const int64_t idleId = idleLease ? idleLease->command("CLIENT ID")->toInt64() : 0;
        idleLease.reset();
        check(idleDestroyPool->availableCount() == 1 && ClientState(redis, idleId, true),
            "returned idle connection is still present on the server");
        idleDestroyPool.reset();
        check(ClientState(redis, idleId, false) && redis.ping(), "idle pool destructor closed the observed session");

        std::atomic<int32_t> threadFailCount(0);
        std::vector<std::thread> threads;
        for (int32_t threadIndex = 0; threadIndex < 6; ++threadIndex)
        {
            threads.push_back(std::thread([threadIndex, &pool, &prefix, &threadFailCount]() -> void
            {
                for (int32_t loopIndex = 0; loopIndex < 40; ++loopIndex)
                {
                    std::shared_ptr<HiRedis> connection = pool.acquire(10000);
                    if (connection.get() == nullptr)
                    {
                        threadFailCount.fetch_add(1);
                        continue;
                    }
                    std::ostringstream keyStream;
                    keyStream << prefix << "thread_" << threadIndex << "_" << loopIndex;
                    std::string key = keyStream.str();
                    if (!connection->set(key, "thread") || connection->get(key)->toString() != "thread")
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
        check(threadFailCount.load() == 0, "HiRedisConnectionPool multithread pressure");
        check(redis.keyExist(prefix + "thread_0_0"), "pooled writes are visible in the configured test database");
        pool.uninit();
    }
    else
    {
        skip("HiRedisConnectionPool multithread pressure");
    }

    std::vector<std::string> cleanKeys;
    cleanKeys.push_back(stringKey);
    cleanKeys.push_back(binaryKey);
    cleanKeys.push_back(rawPercentKey);
    cleanKeys.push_back(rawEscapeKey);
    cleanKeys.push_back(rawTrailingEscapeKey);
    cleanKeys.push_back(nxKey);
    cleanKeys.push_back(intKey);
    cleanKeys.push_back(mapKey1);
    cleanKeys.push_back(mapKey2);
    cleanKeys.push_back(deleteOneKey);
    cleanKeys.push_back(deleteManyKey1);
    cleanKeys.push_back(deleteManyKey2);
    cleanKeys.push_back(hashKey);
    cleanKeys.push_back(tableKey);
    cleanKeys.push_back(prefix + "empty_table");
    cleanKeys.push_back(groupKey);
    cleanKeys.push_back(orderKey);
    cleanKeys.push_back(transactionKey);
    cleanKeys.push_back(prefix + "expire");
    cleanKeys.push_back(prefix + "pipeline");
    for (int32_t threadIndex = 0; threadIndex < 6; ++threadIndex)
    {
        for (int32_t loopIndex = 0; loopIndex < 40; ++loopIndex)
        {
            std::ostringstream keyStream;
            keyStream << prefix << "thread_" << threadIndex << "_" << loopIndex;
            cleanKeys.push_back(keyStream.str());
        }
    }
    redis.del(cleanKeys);
    cleanKeys.insert(cleanKeys.begin(), "EXISTS");
    const auto remaining = redis.command(cleanKeys);
    check(remaining->isValid() && !remaining->isError() && remaining->toInt64() == 0, "all keys created by this test were deleted");
    redis.uninit();
    check(!redis.isConnected(), "HiRedis uninit disconnected");

    std::cout << "HiRedis test " << (failCount == 0 ? "PASS" : "FAIL") <<
        ", total=" << totalCount << ", failed=" << failCount << ", skipped=" << skipCount << std::endl;
    return failCount == 0 ? 0 : 1;
}

/** 按功能用例号或显式基准选项运行
@param [in] argc 参数数量
@param [in] argv 参数列表
@return 成功为 0，检查失败为 1，选择无效为 2
*/
int main(int argc, char** argv)
{
    if (argc == 2 && std::strcmp(argv[1], "--benchmark") == 0)
    {
        try
        {
            RunBenchmark();
            return 0;
        }
        catch (const std::exception& error)
        {
            std::cerr << error.what() << std::endl;
            return 1;
        }
    }
    if (argc == 2 && std::strcmp(argv[1], "--benchmark-db") == 0)
    {
        return RunChecks(true, true);
    }
    LibraryTestCase cases[] = {
        {HiRedisOffline, false, []() { return RunChecks(false, false); }},
        {HiRedisOnline, true, []() { return RunChecks(true, false); }},
        {HiRedisSettings, false, RunSettings},
        {HiRedisPoolRecovery, false, RunPoolRecovery},
        {HiRedisDiscardRecovery, false, []() { return RunTransactionRecovery(false); }},
        {HiRedisExecRecovery, false, []() { return RunTransactionRecovery(true); }},
        {HiRedisDefaultWait, true, RunDefaultWait},
        {HiRedisReaderSettings, false, RunReaderSettings},
        {HiRedisSessionSettings, false, RunSessionSettings},
        {HiRedisConcurrentCapacity, true, RunConcurrentCapacity},
        {HiRedisPoolAuthFailure, false, RunPoolAuthFailure},
        {HiRedisBlockingTimeout, false, RunBlockingTimeout}
    };
    if (argc == 1 || (argc == 2 && std::strcmp(argv[1], "--all") == 0))
    {
        // 全量先瞬时后压力，指定编号时保留按 ID 排列的登记表。
        std::stable_partition(std::begin(cases), std::end(cases), [](const LibraryTestCase& item) { return !item.stress; });
    }
    return LibraryCaseSelection::run(argc, argv, cases);
}