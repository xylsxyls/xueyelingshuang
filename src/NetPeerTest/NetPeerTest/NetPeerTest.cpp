#include "NetPeerTest.h"
#include "NetPeer/NetPeerAPI.h"
#include <memory>
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
    NetPeer& api = NetPeer::instance();
    NetPeerSenderState* client = nullptr;
    CHECK_TEST(api.createClient(client) == NI_OK && client);
    const auto release = [&](NetPeerSenderState* state) { api.destroyClient(state); };
    std::unique_ptr<NetPeerSenderState, decltype(release)> owner(client, release);
    NetPeerSenderState* original = client;
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
    NetPeer& api = NetPeer::instance();
    NetPeerSenderState* client = nullptr;
    CHECK_TEST(api.createClient(client) == NI_OK && client);
    const auto release = [&](NetPeerSenderState* state) { api.destroyClient(state); };
    std::unique_ptr<NetPeerSenderState, decltype(release)> owner(client, release);
    NetPeerSenderState* other = nullptr;
    CHECK_TEST(api.createClient(other) == NI_OK && client != other);
    std::unique_ptr<NetPeerSenderState, decltype(release)> second(other, release);
    CHECK_TEST(api.prepareGateway(client) == NI_OK && api.prepareGateway(other) == NI_OK);
    const uint32_t types[] = {17, 257}, invalid[] = {17, 0};
    CHECK_TEST(api.configureCompression(client, nullptr, 2) == NI_INVALID_ARGUMENT &&
        api.configureCompression(client, invalid, 2) == NI_INVALID_ARGUMENT);
    CHECK_TEST(api.configureCompression(client, types, 2) == NI_OK && api.configureCompression(other, nullptr, 0) == NI_OK);
    CHECK_TEST(api.configureDispatch(client, true) == NI_OK && api.configureDispatch(other, false) == NI_OK);
    return 0;
}

/** 离线验证两个回环候选的去重、轮选和发现停止状态
@return 全部通过为0
*/
static int Discovery()
{
    CHECK_TEST(NetPeer::configureWorkers(0) == NI_INVALID_ARGUMENT && NetPeer::configureWorkers(1) == NI_OK &&
        NetPeer::configureWorkers(2) == NI_NOT_SUPPORTED);
    std::vector<NetGatewayAddress> seeds;
    CHECK_TEST(NetGatewayDiscovery::parseSeeds("127.0.0.1:7001,127.0.0.1:7001,127.0.0.1:7002", seeds) == NI_OK && seeds.size() == 2);
    NetGatewayDiscovery discovery;
    CHECK_TEST(discovery.stopped() && discovery.start(seeds) == NI_OK && discovery.discovered() == 0);
    NetClientDiscoveryPoint first = {}, second = {};
    first.m_structSize = second.m_structSize = sizeof(first);
    first.m_version = second.m_version = 1;
    CHECK_TEST(discovery.choose(first) && discovery.choose(second) && first.m_port != second.m_port &&
        ((first.m_port == 7001 && second.m_port == 7002) || (first.m_port == 7002 && second.m_port == 7001)));
    CHECK_TEST(!std::strcmp(first.m_address, "127.0.0.1") && !std::strcmp(second.m_address, "127.0.0.1"));
    discovery.shutdown();
    CHECK_TEST(discovery.stopped() && !discovery.choose(first));
    CHECK_TEST(NetGatewayDiscovery::parseSeeds("127.0.0.1:0", seeds) == NI_INVALID_ARGUMENT && seeds.empty());
    return 0;
}

/** 度量已准备但未连接的客户端状态读取，逐次检查代次和就绪位
@return 成功为0
*/
static int Benchmark()
{
    NetPeer& api = NetPeer::instance();
    NetPeerSenderState* client = nullptr;
    CHECK_TEST(api.createClient(client) == NI_OK && client);
    const auto release = [&](NetPeerSenderState* state) { api.destroyClient(state); };
    std::unique_ptr<NetPeerSenderState, decltype(release)> owner(client, release);
    CHECK_TEST(api.prepareGateway(client) == NI_OK);
    LibraryBenchmark::run("NetPeer.idleState", [&](uint32_t count) -> uint64_t {
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

/** 种子轮选
@return 全部通过为0
*/
static int RepeatedOperations()
{
    std::vector<NetGatewayAddress> seeds;
    CHECK_TEST(NetGatewayDiscovery::parseSeeds("127.0.0.1:7001,127.0.0.1:7002", seeds) == NI_OK && seeds.size() == 2);
    NetGatewayDiscovery discovery;
    for (uint32_t cycle = 0; cycle < 4; ++cycle)
    {
        CHECK_TEST(discovery.start(seeds) == NI_OK);
        NetClientDiscoveryPoint endpoint = {};
        endpoint.m_structSize = sizeof(endpoint);
        endpoint.m_version = 1;
        uint32_t first = 0;
        for (uint32_t i = 0; i < 512; ++i)
        {
            CHECK_TEST(discovery.choose(endpoint) && !std::strcmp(endpoint.m_address, "127.0.0.1"));
            CHECK_TEST(endpoint.m_port == 7001 || endpoint.m_port == 7002);
            if (endpoint.m_port == 7001)
            {
                ++first;
            }
        }
        CHECK_TEST(first == 256);
        discovery.shutdown();
        CHECK_TEST(!discovery.choose(endpoint) && endpoint.m_port == 0 && endpoint.m_address[0] == 0);
    }
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
        {3, false, Discovery},
        {4, true, RepeatedOperations}
    };
    return LibraryCaseSelection::run(argc, argv, cases);
}
catch (const std::exception& error)
{
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}