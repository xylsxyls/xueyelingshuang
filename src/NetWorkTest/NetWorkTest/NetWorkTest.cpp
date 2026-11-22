#include "NetWorkTest.h"
#include "NetWork/NetWorkAPI.h"
#include <memory>
#ifndef __EMSCRIPTEN__
#include <thread>
#endif
#include "PlatformTest/PlatformTestAPI.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>

#define CHECK_TEST(expr) do { if (!(expr)) { std::fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #expr); return 1; } } while (false)

/** 核对未连接时的代次、准备、请求拒绝及所有权
@return 全部通过为0
*/
static int Lifecycle()
{
    NetWork& api = NetWork::instance();
    NetWorkSenderState* client = nullptr;
    CHECK_TEST(api.createClient(client) == NI_OK && client);
    const auto release = [&](NetWorkSenderState* state) { api.destroyClient(state); };
    std::unique_ptr<NetWorkSenderState, decltype(release)> owner(client, release);
    NetWorkSenderState* original = client;
    CHECK_TEST(api.createClient(client) == NI_INVALID_ARGUMENT && client == original);
    uint64_t generation = 99, request = 99;
    bool ready = true;
    CHECK_TEST(api.gatewayState(client, generation, ready) == NI_OK && generation == 0 && !ready);
    CHECK_TEST(api.prepareGateway(client) == NI_OK && api.prepareGateway(client) == NI_OK);
    CHECK_TEST(api.beginRequest(client, 7, 1, 1000, request) == NI_INVALID_STATE && request == 0);
    CHECK_TEST(!api.gatewayReady(client) && api.gatewayGeneration(client) == 0 && api.pollGateway(client) == NI_OK);
    owner.release();
    CHECK_TEST(api.destroyClient(client) == NI_OK && !client && api.destroyClient(client) == NI_OK);
    return 0;
}

/** 错误配置不妨碍正确配置和独立客户端
@return 全部通过为0
*/
static int Configuration()
{
    NetWork& api = NetWork::instance();
    NetWorkSenderState* client = nullptr;
    CHECK_TEST(api.createClient(client) == NI_OK && client);
    const auto release = [&](NetWorkSenderState* state) { api.destroyClient(state); };
    std::unique_ptr<NetWorkSenderState, decltype(release)> owner(client, release);
    NetWorkSenderState* other = nullptr;
    CHECK_TEST(api.createClient(other) == NI_OK && client != other);
    std::unique_ptr<NetWorkSenderState, decltype(release)> second(other, release);
    CHECK_TEST(api.prepareGateway(client) == NI_OK && api.prepareGateway(other) == NI_OK);
    const uint32_t types[] = {17, 257}, invalid[] = {17, 0};
    CHECK_TEST(api.configureCompression(client, nullptr, 2) == NI_INVALID_ARGUMENT &&
        api.configureCompression(client, invalid, 2) == NI_INVALID_ARGUMENT);
    CHECK_TEST(api.configureCompression(client, types, 2) == NI_OK && api.configureCompression(other, nullptr, 0) == NI_OK);
    CHECK_TEST(api.configureDispatch(client, true) == NI_OK && api.configureDispatch(other, false) == NI_OK);
    return 0;
}

/** 观察静止客户端的线程安全读操作，核对文件执行者的宿主能力
@return 全部通过为0
*/
static int WorkerPolicy()
{
    CHECK_TEST(NetWork::configureWorkers(0) == NI_INVALID_ARGUMENT && NetWork::configureWorkers(UINT32_MAX) == NI_INVALID_ARGUMENT);
    CHECK_TEST(NetWork::configureWorkers(2) == NI_OK && NetWork::pointerBytes() == sizeof(void*));
#ifdef __EMSCRIPTEN__
    CHECK_TEST(NetWork::configureNativeFileIo() == NI_NOT_SUPPORTED);
#ifndef __EMSCRIPTEN_PTHREADS__
    NetWorkClientCallbacks callbacks = {};
    callbacks.structSize = sizeof(callbacks);
    callbacks.version = 1;
    callbacks.ready = [](void*, uint64_t, uint32_t, uint32_t) {};
    callbacks.result = [](void*, uint64_t, uint64_t, uint32_t, uint32_t, uint32_t, const uint8_t*, uint32_t) {};
    callbacks.frame = [](void*, uint64_t, const NetInterfaceFrame*) -> int32_t { return 1; };
    callbacks.closed = [](void*, uint64_t, int32_t, int32_t) {};
    NetWorkClientHandle handle = nullptr;
    CHECK_TEST(NetWork::instance().clientCreate(&callbacks, &handle) == NI_OK && handle);
    NetWorkClientOptions options = {};
    options.structSize = sizeof(options);
    options.version = 1;
    options.address = "127.0.0.1";
    options.port = 5203;
    const int32_t result = NetWork::instance().clientConnect(handle, &options);
    CHECK_TEST(NetWork::instance().clientDestroy(handle) == NI_OK && result == NI_NOT_SUPPORTED);
#endif
#else
    CHECK_TEST(NetWork::configureNativeFileIo() == NI_OK);
#endif
    NetWork& api = NetWork::instance();
    NetWorkSenderState* client = nullptr;
    CHECK_TEST(api.createClient(client) == NI_OK && client);
    const auto release = [&](NetWorkSenderState* state) { api.destroyClient(state); };
    std::unique_ptr<NetWorkSenderState, decltype(release)> owner(client, release);
    CHECK_TEST(api.prepareGateway(client) == NI_OK);
    const auto stable = [&]() -> bool {
        for (int i = 0; i < 2000; ++i)
        {
            uint64_t generation = 7;
            bool ready = true;
            if (api.gatewayState(client, generation, ready) != NI_OK || generation != 0 || ready)
            {
                return false;
            }
        }
        return true;
    };
#ifndef __EMSCRIPTEN__
    bool other = false;
    std::thread reader([&]() { other = stable(); });
    const bool local = stable();
    reader.join();
    CHECK_TEST(local && other);
#else
    CHECK_TEST(stable());
#endif
    return 0;
}

/** 度量已准备但未连接的客户端状态读取，逐次检查代次和就绪位
@return 成功为0
*/
static int Benchmark()
{
    NetWork& api = NetWork::instance();
    NetWorkSenderState* client = nullptr;
    CHECK_TEST(api.createClient(client) == NI_OK && client);
    const auto release = [&](NetWorkSenderState* state) { api.destroyClient(state); };
    std::unique_ptr<NetWorkSenderState, decltype(release)> owner(client, release);
    CHECK_TEST(api.prepareGateway(client) == NI_OK);
    LibraryBenchmark::run("NetWork.idleState", [&](uint32_t count) -> uint64_t {
        uint64_t valid = 0;
        for (uint32_t i = 0; i < count; ++i)
        {
            uint64_t generation = 17;
            bool ready = true;
            valid += api.gatewayState(client, generation, ready) == NI_OK && generation == 0 && !ready;
        }
        if (valid != count)
        {
            throw std::runtime_error("idleState");
        }
        return valid;
    });
    return 0;
}

/** 按稳定编号选择用例，返回库测试调度器的退出码
@param [in] argc 参数个数
@param [in] argv 命令行参数
@return 全部通过为0，断言失败为1，参数错误为2
*/
int main(int argc, char** argv)
try
{
    if (argc == 2 && std::strcmp(argv[1], "--benchmark") == 0)
    {
        return Benchmark();
    }
    const LibraryTestCase cases[] = {
        {1, false, Lifecycle},
        {2, false, Configuration},
        {3, true, WorkerPolicy}
    };
    return LibraryCaseSelection::run(argc, argv, cases);
}
catch (const std::exception& error)
{
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}