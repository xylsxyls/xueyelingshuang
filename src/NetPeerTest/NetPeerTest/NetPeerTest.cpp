#include "NetPeerTest.h"
#include "NetSender/NetSenderAPI.h"
#include "NetPeer/NetPeerClientState.h"
#include <algorithm>
#include <memory>
#include <new>
#if defined(__EMSCRIPTEN__)
#include "NetPeer/NetPeerBindings.h"
#include <emscripten/emscripten.h>
#endif
#include "NetGatewayTest/NetTestTlsCertificateData.h"
#include "PlatformTest/PlatformTestAPI.h"
#include <cstdio>
#include <cstring>
#include <exception>
#include <stdexcept>
#include "ProtoMessageManager/ProtoMessageManagerAPI.h"

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

#define NET_PEER_TEST_STRING_LITERAL(value) #value
#define NET_PEER_TEST_EXPAND_LITERAL(value) NET_PEER_TEST_STRING_LITERAL(value)

/** 按 SDK 选定的链接方式解析测试种子，跨模块时仅由测试程序分配和释放输出容器
@param [in] text 逗号分隔的数值地址和端口，借用至返回
@param [out] seeds 成功时为去重后的候选，失败清空
@return 解析结果或负数错误
*/
static int32_t ParseTestSeeds(const std::string& text, std::vector<NetGatewayAddress>& seeds)
{
    // SDK 静态版本的 NetPeerAPI 展开为空，其余版本均使用固定布局输出。
    if (sizeof(NET_PEER_TEST_EXPAND_LITERAL((NetPeerAPI))) == sizeof("()"))
    {
        return NetGatewayDiscovery::parseSeeds(text, seeds);
    }
    uint32_t count = 0;
    const int32_t measured = NetClientDiscovery::parseSeeds(text, nullptr, 0, count);
    if (measured != NI_BUFFER_TOO_SMALL)
    {
        seeds.clear();
        return measured;
    }
    std::vector<NetClientDiscoveryPoint> points(count);
    for (auto& point : points)
    {
        point.m_structSize = sizeof(point);
        point.m_version = 1;
    }
    const int32_t result = NetClientDiscovery::parseSeeds(text, points.data(), count, count);
    seeds.clear();
    if (result == NI_OK)
    {
        for (uint32_t index = 0; index < count; ++index)
        {
            seeds.push_back(NetGatewayAddress{points[index].m_address, points[index].m_port});
        }
    }
    return result;
}

#undef NET_PEER_TEST_EXPAND_LITERAL
#undef NET_PEER_TEST_STRING_LITERAL

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
    NetConnectionPoolConfig connection;
    connection.m_progressFrameBudget = 0;
    CHECK_TEST(api.configureConnectionPools(client, connection) == NI_INVALID_ARGUMENT &&
        api.lastGatewayError(client) == NI_INVALID_ARGUMENT);
    connection.m_progressFrameBudget = 1;
    CHECK_TEST(api.configureConnectionPools(client, connection) == NI_OK);
    LockFreeHashMapPoolConfig request;
    request.initialCapacity = 2;
    request.maximumCapacity = 1;
    CHECK_TEST(api.configureRequestPool(client, request) == NI_INVALID_ARGUMENT);
    request.maximumCapacity = 2;
    CHECK_TEST(api.configureRequestPool(client, request) == NI_OK);
    NetSenderSubmissionPools submission;
    submission.m_receipts.initialCapacity = 2;
    submission.m_receipts.maximumCapacity = 1;
    CHECK_TEST(api.configureSubmissionPools(client, submission) == NI_INVALID_ARGUMENT);
    submission.m_receipts.maximumCapacity = 2;
    CHECK_TEST(api.configureSubmissionPools(client, submission) == NI_OK && api.lastGatewayError(client) == NI_OK);
    NetSenderSubmissionOptions submissionOptions;
    submissionOptions.m_inputBatchSize = 33;
    CHECK_TEST(api.configureSubmissionPools(client, submission, submissionOptions) == NI_INVALID_ARGUMENT);
    submissionOptions.m_inputBatchSize = 2;
    submissionOptions.m_deliveryTimeoutMs = 0;
    CHECK_TEST(NetSender::backend().configureSubmissionPools(other, submission, submissionOptions) == NI_INVALID_ARGUMENT);
    submissionOptions.m_deliveryTimeoutMs = 1000;
    CHECK_TEST(NetSender::backend().configureSubmissionPools(other, submission, submissionOptions) == NI_OK);
    CHECK_TEST(api.configureSubmissionPools(client, submission, submissionOptions) == NI_OK);
    const uint32_t types[] = {17, 257}, invalid[] = {17, 0};
    CHECK_TEST(api.configureCompression(client, nullptr, 2) == NI_INVALID_ARGUMENT &&
        api.configureCompression(client, invalid, 2) == NI_INVALID_ARGUMENT);
    CHECK_TEST(api.configureCompression(client, types, 2) == NI_OK && api.configureCompression(other, nullptr, 0) == NI_OK);
    CHECK_TEST(api.configureDispatch(client, true) == NI_OK && api.configureDispatch(other, false) == NI_OK);
    // 恰好4097个可读字节，无终止符；有界入口应在调用解析器之前拒绝。
    const std::vector<char> oversized(4097, 'x');
    const std::string longestPath(4096, 'p');
    CHECK_TEST(api.configureGateways(client, oversized.data(), nullptr) == NI_INVALID_ARGUMENT);
    CHECK_TEST(api.configureGateways(client, "127.0.0.1:9123", oversized.data()) == NI_INVALID_ARGUMENT);
    CHECK_TEST(api.configureGateways(client, "127.0.0.1:9123", longestPath.c_str()) == NI_OK);
    CHECK_TEST(api.configureGateways(client, nullptr, nullptr) == NI_OK);
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
    CHECK_TEST(ParseTestSeeds("127.0.0.1:7001,127.0.0.1:7001,127.0.0.1:7002", seeds) == NI_OK && seeds.size() == 2);
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
    NetInterfaceTlsOptions tls = {};
    for (uint32_t invalid : {99U, 60001U}) {
        tls.handshakeTimeoutMs = invalid;
        CHECK_TEST(discovery.start(seeds, "", tls) == NI_INVALID_ARGUMENT && discovery.stopped());
    }
    for (uint32_t allowed : {0U, 100U, 60000U}) {
        tls.handshakeTimeoutMs = allowed;
        CHECK_TEST(discovery.start(seeds, "", tls) == NI_OK && discovery.choose(first));
        discovery.shutdown();
        CHECK_TEST(discovery.stopped());
    }
    std::vector<NetGatewayAddress> invalidSeeds(seeds);
    invalidSeeds.front().m_address.append(1, '\0');
    invalidSeeds.front().m_address += "suffix";
    CHECK_TEST(discovery.start(invalidSeeds) == NI_INVALID_ARGUMENT && discovery.stopped());
    NetGatewayDirectoryStore invalidCache;
    CHECK_TEST(invalidCache.open(std::string("prefix\0suffix", 13)) == NI_INVALID_ARGUMENT && invalidCache.stopped());
    NetGatewayDiscoveryConfig invalidLink;
    invalidLink.m_transport.m_link.m_controlStepBudget = 0;
    NetGatewayDiscovery invalidProbe(invalidLink);
    CHECK_TEST(invalidProbe.start(seeds) == NI_INVALID_ARGUMENT && invalidProbe.stopped());
    CHECK_TEST(ParseTestSeeds("127.0.0.1:0", seeds) == NI_INVALID_ARGUMENT && seeds.empty());
    return 0;
}

static PeerTestWire* activeWire = nullptr;
static bool failEntropy = false;
// 下一次模拟拨号从宿主抛出，验证资源与输出参数的收敛。
static bool failConnect = false;
// 仅同步拨号用例安装此事件序列，借用到本次 connect 返回。
static std::function<void(PeerTestWire&, uint64_t&)> synchronousDial;

/** 安装测试进程的单执行者宿主，每项可在独立进程中运行
@return 宿主登记成功为0
*/
static int InstallWire()
{
    static bool installed = false;
    if (installed) { return NI_OK; }
    NetHostFunctions host = {};
    host.structSize = sizeof(host);
    host.version = 1;
    host.create = [](void*, void* observer, NetHostConnected connected, NetHostData data, NetHostWritable writable,
        NetHostClosed closed, void** out) -> int32_t {
        PeerTestWire* wire = new (std::nothrow) PeerTestWire;
        if (!wire) { return -1; }
        wire->observer = observer;
        wire->connected = connected;
        wire->data = data;
        wire->writable = writable;
        wire->closed = closed;
        activeWire = wire;
        *out = wire;
        return 0;
    };
    host.listen = [](void* instance, const char*, uint32_t port, uint32_t) -> int32_t {
        static_cast<PeerTestWire*>(instance)->boundPort = port == 0 ? 40123 : port;
        return 0;
    };
    host.connect = [](void* instance, const char*, uint32_t, uint64_t* peer) -> int32_t {
        if (failConnect) { failConnect = false; throw std::runtime_error("connect"); }
        PeerTestWire& wire = *static_cast<PeerTestWire*>(instance);
        wire.live = true;
        *peer = 1;
        if (synchronousDial)
        {
            synchronousDial(wire, *peer);
        }
        return 0;
    };
    host.poll = [](void* p) -> int32_t {
        const PeerTestWire* wire = static_cast<PeerTestWire*>(p);
        return wire->live || wire->boundPort != 0 ? 1 : 0;
    };
    host.pauseRead = [](void* p, uint64_t, int32_t paused) -> int32_t {
        static_cast<PeerTestWire*>(p)->paused = paused != 0;
        return 0;
    };
    host.write = [](void* p, uint64_t, const uint8_t* data, uint32_t size) -> int32_t {
        PeerTestWire* wire = static_cast<PeerTestWire*>(p);
        if (wire->m_blockWrites)
        {
            ++wire->m_writeRefusals;
            return 1;
        }
        if (size > wire->m_largestWrite) { wire->m_largestWrite = size; }
        if (wire->capture && size) { wire->output.insert(wire->output.end(), data, data + size); }
        wire->writes += size;
        return 0;
    };
    host.close = [](void* p, uint64_t peer, int32_t reason) -> int32_t {
        PeerTestWire* w = static_cast<PeerTestWire*>(p);
        ++w->closes;
        if (w->m_closeReason)
        {
            *w->m_closeReason = reason;
        }
        if (w->live) { w->live = false; w->closed(w->observer, peer, reason); }
        return 0;
    };
    host.shutdown = [](void* p) -> int32_t {
        PeerTestWire* w = static_cast<PeerTestWire*>(p);
        w->boundPort = 0;
        if (w->live) { w->live = false; w->closed(w->observer, 1, 0); }
        return 0;
    };
    host.destroy = [](void* p) -> int32_t {
        if (p == activeWire) { activeWire = nullptr; }
        delete static_cast<PeerTestWire*>(p);
        return 0;
    };
    host.localPort = [](void* p) -> uint32_t { return static_cast<PeerTestWire*>(p)->boundPort; };
    host.peerCount = [](void* p) -> uint32_t { return static_cast<PeerTestWire*>(p)->live ? 1U : 0U; };
    host.entropy = [](void*, uint8_t* out, uint32_t size) -> int32_t {
        if (failEntropy) { if (size) { out[0] = 0xcc; } return -1; }
        for (uint32_t i = 0; i < size; ++i) { out[i] = static_cast<uint8_t>(i + 7); }
        return 0;
    };
    const int result = NetPeer::backend().configureHost(host);
    installed = result == NI_OK;
    return result;
}

/** 从零槽按需增长；通过回调观察实际使用的是哪一份配置
@param [in,out] config 当前物理池
@param [in,out] count 增长槽数，须覆盖池的生命周期
*/
static void ObservePool(LockFreeHashMapPoolConfig& config, size_t& count)
{
    config.setInitialCapacity(0);
    config.setMaximumCapacity(-1);
    config.setGrowFreeThreshold(0);
    config.setGrowPolicy([](const LockFreeHashMapGrowContext&) -> size_t { return 1; });
    config.setGrowthObserver([&count](size_t added, uint64_t) { count += added; });
}

/** 验证配置事务不会发布失败对象，空闲状态接受更正
@return 全部通过为0
*/
static int PreparedConfiguration()
{
    NetPeer& api = NetPeer::instance();
    NetPeerSenderState* client = nullptr;
    CHECK_TEST(api.createClient(client) == NI_OK);
    const auto release = [&](NetPeerSenderState* state) { api.destroyClient(state); };
    std::unique_ptr<NetPeerSenderState, decltype(release)> owner(client, release);
    CHECK_TEST(api.configureDispatch(client, true) == NI_INVALID_STATE);
    NetPeerConfig config;
    PeerPreparationProbe probe = {client, NI_OK};
    config.m_context = &probe;
    config.m_configure = [](void* context, NetClientSession&, NetSenderDispatch&) -> int32_t {
        PeerPreparationProbe& probe = *static_cast<PeerPreparationProbe*>(context);
        probe.m_nested = NetPeer::instance().prepareGateway(probe.m_client);
        return NI_INVALID_ARGUMENT;
    };
    CHECK_TEST(api.prepareGateway(client, config) == NI_INVALID_ARGUMENT && probe.m_nested == NI_INVALID_STATE);
    CHECK_TEST(api.configureDispatch(client, true) == NI_INVALID_STATE);
    config.m_configure = [](void*, NetClientSession&, NetSenderDispatch&) -> int32_t { return NI_READY; };
    CHECK_TEST(api.prepareGateway(client, config) == NI_INVALID_STATE);
    CHECK_TEST(api.configureDispatch(client, true) == NI_INVALID_STATE);
    config.m_configure = [](void* context, NetClientSession&, NetSenderDispatch&) -> int32_t {
        PeerPreparationProbe& probe = *static_cast<PeerPreparationProbe*>(context);
        NetPeerConfig nestedConfig;
        probe.m_nested = NetPeer::instance().prepareGateway(probe.m_client, nestedConfig);
        return NI_INVALID_ARGUMENT;
    };
    CHECK_TEST(api.prepareGateway(client, config) == NI_INVALID_ARGUMENT && probe.m_nested == NI_INVALID_STATE);
    config.m_configure = [](void* context, NetClientSession&, NetSenderDispatch&) -> int32_t {
        PeerPreparationProbe& probe = *static_cast<PeerPreparationProbe*>(context);
        probe.m_nested = NetPeer::instance().destroyClient(probe.m_client);
        return NI_INVALID_ARGUMENT;
    };
    CHECK_TEST(api.prepareGateway(client, config) == NI_INVALID_ARGUMENT && probe.m_nested == NI_INVALID_STATE && probe.m_client == client);
    unsigned calls = 0;
    config.m_context = &calls;
    config.m_configure = [](void* context, NetClientSession& session, NetSenderDispatch& dispatch) -> int32_t {
        unsigned& count = *static_cast<unsigned*>(context);
        ++count;
        if (count == 1) { return session.configureExpiryBudget(0); }
        NetHeartbeatConfig heartbeat;
        heartbeat.m_pingIntervalMs = 1000;
        CEncodeDecodeTlsQueueLimits ciphertext;
        if (!ciphertext.set(32768, -1) || session.configureHeartbeat(heartbeat) != NI_OK ||
            session.configureServiceWindow(65536) != NI_OK || session.configureServiceTlsQueueLimits(ciphertext) != NI_OK ||
            session.configureServicePollBudgets(4, 7) != NI_OK || session.configureServiceTimeouts(500, 250) != NI_OK ||
            session.configureRecovery(15000, 100, 500) != NI_OK || session.configurePayloadRetention(-1) != NI_OK ||
            session.configureExpiryBudget(3) != NI_OK || dispatch.configurePollBudgets(2, 2048) != NI_OK) { return NI_INVALID_STATE; }
        NetSenderSubmissionPools pools;
        NetSenderSubmissionOptions options;
        options.m_inputBatchSize = 3;
        return dispatch.submission().configurePools(pools, options);
    };
    CHECK_TEST(api.prepareGateway(client, config) == NI_INVALID_ARGUMENT && calls == 1);
    CHECK_TEST(api.lastGatewayError(client) == NI_INVALID_ARGUMENT && api.configureDispatch(client, false) == NI_INVALID_STATE);
    CHECK_TEST(api.prepareGateway(client, config) == NI_OK && calls == 2);
    CHECK_TEST(api.prepareGateway(client, config) == NI_INVALID_STATE && calls == 2);
    CHECK_TEST(api.prepareGateway(client) == NI_OK && api.configureDispatch(client, false) == NI_OK);
    return 0;
}

void PeerTestListener::onReady(uint64_t)
{ ++readyCount; }

bool PeerTestListener::onFrame(uint64_t, const NetInterfaceFrame& frame)
{
    if (m_rejectFrame)
    {
        ++m_refusedFrames;
        return false;
    }
    frames.push_back(frame.m_requestId);
    if (frame.m_payloadSize == 3 && frame.m_payload[1] == 5 && frame.m_payload[2] == 4) { bodies.push_back(frame.m_payload[0]); }
    return true;
}

void PeerTestListener::onConnected(uint64_t)
{ ++connectedCount; }

void PeerTestListener::onData(uint64_t, const uint8_t* data, uint32_t size)
{ ++inputs; input.assign(data, data + size); }

void PeerTestListener::onWritable(uint64_t)
{ ++writableCount; }

void PeerTestListener::onClosed(uint64_t, int32_t reason, int32_t detail)
{ ++closeCount; closeReason = reason; closeDetail = detail; }

void PeerTestListener::onRelayConnected(uint64_t)
{ ++connectedCount; }

bool PeerTestListener::onRelayRecord(uint64_t, const NetInterfaceTunnelRecord& record)
{
    ++m_relayOffers;
    if (m_rejectRelayRecord) { return false; }
    m_relayBody.clear();
    if (record.m_size) { m_relayBody.assign(record.m_data, record.m_data + record.m_size); }
    return true;
}

void PeerTestListener::onRelayClosed(uint64_t, int32_t reason)
{
    ++closeCount;
    closeReason = reason;
}

void PeerTestListener::onLinkReady(uint64_t, const NetInterfaceLinkPeer&)
{
    ++readyCount;
}

bool PeerTestListener::onLinkControl(uint64_t, const NetInterfaceFrame& frame)
{
    if (frame.m_requestId > m_acceptControlThrough)
    {
        ++m_refusedControl;
        return false;
    }
    // 零号标记格式错误的发现请求，调用方按收到的完整顺序核对。
    frames.push_back(frame.m_kind == NI_REQUEST && frame.m_messageType == NI_GATEWAY_DISCOVER &&
        frame.m_productId == 0 ? frame.m_requestId : 0);
    return true;
}

bool PeerTestListener::onLinkRecord(uint64_t peer, const NetInterfaceTunnelRecord& record)
{
    return onRelayRecord(peer, record);
}

void PeerTestListener::onLinkClosed(uint64_t, int32_t reason)
{
    ++closeCount;
    closeReason = reason;
}

#if defined(__EMSCRIPTEN__)
/** 经真实C回调表交付无符号边界，核对Wasm到JS的整数和字节保真
@param [in] callbacks JS Sender登记的有效v1回调表，调用期间借用
@param [in] step 0至3分别选择低值、有符号最大值、最高位和无符号最大值
@return 全部入口可调用并且frame被接纳时返回0
*/
extern "C" EMSCRIPTEN_KEEPALIVE int netPeerTestCallbackIntegers(const NetInterfaceClientCallbacks* callbacks, uint32_t step)
{
    CHECK_TEST(callbacks && callbacks->structSize == sizeof(*callbacks) && callbacks->version == 1 && step < 4);
    CHECK_TEST(callbacks->ready && callbacks->result && callbacks->frame && callbacks->closed);
    const uint32_t values32[] = { 1, 0x7fffffffU, 0x80000000U, 0xffffffffU };
    const uint64_t values64[] = { 1, 0x7fffffffffffffffULL, 0x8000000000000000ULL, 0xffffffffffffffffULL };
    uint8_t bytes[] = { 0, 83, 255 };
    const uint64_t generation = values64[step];
    const uint64_t request = values64[3 - step];
    NetInterfaceFrame frame = {};
    frame.m_productId = values32[step];
    frame.m_messageType = values32[3 - step];
    frame.m_requestId = request;
    frame.m_kind = NI_DATA;
    frame.m_payload = bytes;
    frame.m_payloadSize = sizeof(bytes);
    callbacks->ready(callbacks->context, generation, values32[step], values32[3 - step]);
    callbacks->result(callbacks->context, generation, request, frame.m_productId, frame.m_messageType, 0, bytes, sizeof(bytes));
    CHECK_TEST(callbacks->frame(callbacks->context, generation, &frame) != 0);
    callbacks->closed(callbacks->context, generation, -7, -83);
    std::fill(bytes, bytes + sizeof(bytes), 0);
    return 0;
}

/** 经真实监听表先通知就绪，再交付失败 ACK 和无关数据帧
@param [in] callbacks 已准备的当前 Wasm 客户端监听表
@param [in] reason 接收端合法失败原因
@param [in] wrongToken 非零时构造令牌错配的反例
@return 全部回调接纳为0
*/
extern "C" EMSCRIPTEN_KEEPALIVE int netPeerTestFileAck(const NetInterfaceClientCallbacks* callbacks, int32_t reason, int32_t wrongToken)
{
    CHECK_TEST(callbacks && callbacks->ready && callbacks->frame);
    callbacks->ready(callbacks->context, 9, 4, 1024);
    NetInterfaceFileAck ack = {};
    ack.flags = 2;
    ack.result = reason;
    ack.token[0] = wrongToken ? 3 : 2;
    uint8_t bytes[NET_INTERFACE_FILE_ACK_SIZE] = {};
    CHECK_TEST(NetInterfaceFile::encodeAck(ack, bytes, sizeof(bytes)) == NI_OK);
    NetInterfaceFrame frame = {7, 61, NI_FILE_DATA_TYPE, NI_DATA_ACK, sizeof(bytes), bytes};
    CHECK_TEST(callbacks->frame(callbacks->context, 9, &frame) != 0);
    if (!wrongToken)
    {
        frame.m_kind = NI_DATA;
        frame.m_requestId = 62;
        frame.m_payloadSize = 1;
        CHECK_TEST(callbacks->frame(callbacks->context, 9, &frame) != 0);
    }
    return 0;
}

/** 经公开随机接口核对宿主交付的两段数据和调用边界
@param [in] size 请求字节数，最多8192
@param [in] prefix 前段字节数
@param [in] first,second 前段和余段各自的预期字节
@param [in] expected 接口预期返回码
@return 全部符合预期返回0
*/
extern "C" EMSCRIPTEN_KEEPALIVE int netPeerTestEntropy(uint32_t size, uint32_t prefix, int first, int second, int expected)
{
    uint8_t bytes[8194];
    CHECK_TEST(size <= 8192 && prefix <= size);
    std::fill(bytes, bytes + size + 2, 0x5a);
    CHECK_TEST(NetPeer::randomBytes(bytes + 1, size) == expected);
    CHECK_TEST(bytes[0] == 0x5a && bytes[size + 1] == 0x5a);
    for (uint32_t i = 0; i < size; ++i)
    {
        CHECK_TEST(bytes[i + 1] == (i < prefix ? first : second));
    }
    return 0;
}

/** 从 JS 宿主测试驱动两个真实字节通道，分阶段观察背压、异步归还与关闭
@param [in] step 0建立并尝试拒收，1处理真实可写并填满共享池，2归还后重试，3收敛，4重新建连，5核对错误后关闭，6至8核对暂停时的EOF恢复与超时，9触发同步写故障，10至12按单事件预算读取两段尾部与关闭，负数清理
@return 各项符合预期为0
*/
extern "C" EMSCRIPTEN_KEEPALIVE int netPeerTestRawHost(int step)
{
    static PeerTestListener listeners[2];
    static std::unique_ptr<NetHostSocket> sockets[2];
    static uint64_t peers[2] = {};
    static const uint8_t bytes[32768] = {};
    if (step < 0)
    {
        for (auto& socket : sockets) { socket.reset(); }
        return 0;
    }
    if (step == 0)
    {
        for (size_t i = 0; i < 2; ++i)
        {
            CHECK_TEST(!sockets[i]);
            listeners[i] = PeerTestListener();
            sockets[i].reset(new NetHostSocket());
            CHECK_TEST(sockets[i]->connect("127.0.0.1", 9000, &listeners[i], peers[i]) == NI_OK);
            CHECK_TEST(sockets[i]->poll() && listeners[i].connectedCount == 1);
        }
        CHECK_TEST(sockets[0]->write(peers[0], bytes, 1) == NI_LIMIT_EXCEEDED);
        CHECK_TEST(sockets[0]->poll() && listeners[0].writableCount == 0);
    }
    else if (step == 1)
    {
        CHECK_TEST(sockets[0] && sockets[1]);
        CHECK_TEST(sockets[0]->poll() && listeners[0].writableCount == 1 && !listeners[0].inputs);
        CHECK_TEST(sockets[0]->write(peers[0], bytes, sizeof(bytes)) == NI_OK);
        CHECK_TEST(sockets[1]->write(peers[1], bytes, 1) == NI_LIMIT_EXCEEDED);
        CHECK_TEST(sockets[1]->poll() && listeners[1].writableCount == 0 && !listeners[1].inputs);
    }
    else if (step == 2)
    {
        CHECK_TEST(sockets[1] && sockets[1]->poll() && listeners[1].writableCount == 1 && !listeners[1].inputs);
        CHECK_TEST(sockets[1]->write(peers[1], bytes, 1) == NI_OK);
        CHECK_TEST(sockets[0]->poll() && listeners[0].writableCount == 2 && !listeners[0].inputs);
        for (size_t i = 0; i < 2; ++i)
        {
            CHECK_TEST(sockets[i]->pauseRead(peers[i], false) == NI_OK && sockets[i]->poll() && sockets[i]->poll());
            CHECK_TEST(listeners[i].inputs == 1 && listeners[i].input.size() == 1 &&
                listeners[i].input[0] == (i == 0 ? 19 : 83) && listeners[i].writableCount == (i == 0 ? 2U : 1U));
        }
    }
    else if (step == 3)
    {
        CHECK_TEST(sockets[0] && sockets[1]);
        sockets[0]->close(peers[0], NI_CANCELLED);
        CHECK_TEST(!sockets[0]->poll() && listeners[0].closeCount == 1 && listeners[0].closeReason == NI_CANCELLED);
        CHECK_TEST(!sockets[1]->poll() && listeners[1].closeCount == 1 && listeners[1].closeDetail == -83);
        for (auto& socket : sockets) { CHECK_TEST(socket->uninit() == NI_OK); socket.reset(); }
    }
    else if (step == 4)
    {
        listeners[0] = PeerTestListener();
        sockets[0].reset(new NetHostSocket());
        CHECK_TEST(sockets[0]->connect("127.0.0.1", 9001, &listeners[0], peers[0]) == NI_OK);
        CHECK_TEST(sockets[0]->poll() && listeners[0].connectedCount == 1);
    }
    else if (step == 5)
    {
        CHECK_TEST(sockets[0] && !sockets[0]->poll() && listeners[0].closeCount == 1 && listeners[0].closeDetail == -91);
        CHECK_TEST(sockets[0]->uninit() == NI_OK);
        sockets[0].reset();
    }
    else if (step == 9)
    {
        CHECK_TEST(sockets[0] && sockets[0]->write(peers[0], bytes, 1) == NI_LIMIT_EXCEEDED);
    }
    else if (step == 10 || step == 11)
    {
        CHECK_TEST(sockets[0] && sockets[0]->pauseRead(peers[0], false) == NI_OK && sockets[0]->poll());
        CHECK_TEST(listeners[0].inputs == (step == 10 ? 1U : 2U) && listeners[0].input.size() == 1 &&
            listeners[0].input[0] == (step == 10 ? 19 : 83) && !listeners[0].closeCount);
        CHECK_TEST(sockets[0]->write(peers[0], bytes, 1) == NI_LIMIT_EXCEEDED);
    }
    else if (step == 12)
    {
        CHECK_TEST(sockets[0] && !sockets[0]->poll() && !sockets[0]->poll());
        CHECK_TEST(listeners[0].inputs == 2 && listeners[0].closeCount == 1 &&
            listeners[0].closeReason == NI_INVALID_STATE && listeners[0].closeDetail == -83);
        CHECK_TEST(sockets[0]->write(peers[0], bytes, 1) == NI_INVALID_STATE && sockets[0]->uninit() == NI_OK);
        sockets[0].reset();
    }
    else if (step == 6)
    {
        CHECK_TEST(sockets[0] && sockets[0]->poll() && sockets[0]->poll());
        CHECK_TEST(!listeners[0].inputs && !listeners[0].closeCount);
    }
    else if (step == 7 || step == 8)
    {
        CHECK_TEST(sockets[0]);
        if (step == 7) { CHECK_TEST(sockets[0]->pauseRead(peers[0], false) == NI_OK); }
        CHECK_TEST(!sockets[0]->poll() && !sockets[0]->poll() && listeners[0].closeCount == 1);
        if (step == 7)
        {
            CHECK_TEST(listeners[0].inputs == 1 && listeners[0].input.size() == 2 &&
                listeners[0].input[0] == 19 && listeners[0].input[1] == 83 && listeners[0].closeReason == NI_OK);
        }
        else
        {
            CHECK_TEST(!listeners[0].inputs && listeners[0].closeReason == NI_INVALID_STATE && listeners[0].closeDetail == NI_TRUNCATED);
        }
        CHECK_TEST(sockets[0]->uninit() == NI_OK);
        sockets[0].reset();
    }
    else { return 1; }
    return 0;
}
#endif

/** 在真实TLS状态机与可控字节宿主之间观察异步拨号及每轮写预算
@return 全部通过为0
*/
static int DirectTransport()
{
    CHECK_TEST(InstallWire() == NI_OK);
    static const uint8_t ca[] = TEST_CA_CRT_RSA_SHA256_PEM;
    NetInterfaceTlsOptions tls = {};
    tls.authority = ca;
    tls.authoritySize = sizeof(ca);
    tls.hostname = "localhost";
    PeerTestListener listener;
    size_t rawGrowth = 0, tlsGrowth = 0;
    NetDirectTransportConfig options;
    ObservePool(options.m_host.m_peers, rawGrowth);
    LockFreeHashMapPoolConfig replacement;
    ObservePool(replacement, tlsGrowth);
    options.m_outputBudget = 11;
    NetTransport transport(options);
    CHECK_TEST(transport.configurePeerPool(replacement) == NI_OK);
    uint64_t id = 0;
    CHECK_TEST(transport.connect("127.0.0.1", 9123, tls, &listener, id) == NI_OK && id == 1 && activeWire);
    CHECK_TEST(tlsGrowth == 1 && rawGrowth == 1 && transport.poll() && activeWire->closes == 0);
    activeWire->connected(activeWire->observer, id);
    CHECK_TEST(transport.poll() && activeWire->writes == 11);
    CHECK_TEST(transport.poll() && activeWire->writes == 22);
    transport.shutdown();
    CHECK_TEST(transport.peerCount() == 0);
    return 0;
}

/** 通过实际 TLS 连接的宿主读写行为核对两个方向的密文队列及实例隔离
@return 两个实例的写入字节数和读背压均符合各自的设置时返回0
*/
static int TlsQueueLimits()
{
    CHECK_TEST(InstallWire() == NI_OK);
    static const uint8_t ca[] = TEST_CA_CRT_RSA_SHA256_PEM;
    NetInterfaceTlsOptions tls = {};
    tls.authority = ca;
    tls.authoritySize = sizeof(ca);
    tls.hostname = "localhost";
    PeerTestListener listener;
    NetDirectTransportConfig config;
    config.m_frameBudget = 1;
    config.m_outputBudget = 11;
    CHECK_TEST(config.m_ciphertext.set(1, 3));
    CHECK_TEST(!config.m_ciphertext.set(0, 7));
    NetTransport limited(config);
    CHECK_TEST(config.m_ciphertext.set(-1, -1));
    NetTransport unlimited(config);
    CHECK_TEST(config.m_ciphertext.set(9, 9));
    uint64_t first = 0;
    uint64_t second = 0;
    CHECK_TEST(limited.connect("127.0.0.1", 9123, tls, &listener, first) == NI_OK && activeWire);
    PeerTestWire* limitedWire = activeWire;
    CHECK_TEST(unlimited.connect("127.0.0.1", 9124, tls, &listener, second) == NI_OK && activeWire != nullptr && activeWire != limitedWire);
    PeerTestWire* unlimitedWire = activeWire;
    limitedWire->connected(limitedWire->observer, first);
    unlimitedWire->connected(unlimitedWire->observer, second);
    CHECK_TEST(limited.poll() && limitedWire->writes == 3);
    CHECK_TEST(unlimited.poll() && unlimitedWire->writes == 11);
    const uint8_t recordPrefix[] = {22, 3, 3};
    limitedWire->data(limitedWire->observer, first, recordPrefix, sizeof(recordPrefix));
    unlimitedWire->data(unlimitedWire->observer, second, recordPrefix, sizeof(recordPrefix));
    CHECK_TEST(limited.poll() && limitedWire->paused && limitedWire->closes == 0);
    CHECK_TEST(unlimited.poll() && !unlimitedWire->paused && unlimitedWire->closes == 0);
    limited.shutdown();
    unlimited.shutdown();
    CHECK_TEST(limited.peerCount() == 0 && unlimited.peerCount() == 0);
    return 0;
}

/** 通过实际服务运输和发现器发起连接，分别观察宿主及管理槽的增长
@return 全部通过为0
*/
static int LinkBudgets()
{
    CHECK_TEST(InstallWire() == NI_OK);
    size_t service = 0, discovery = 0, serviceRaw = 0, discoveryRaw = 0;
    NetPeerConfig cfg;
    ObservePool(cfg.m_service.m_peers, service);
    ObservePool(cfg.m_service.m_host.m_peers, serviceRaw);
    ObservePool(cfg.m_discovery.m_transport.m_peers, discovery);
    ObservePool(cfg.m_discovery.m_transport.m_host.m_peers, discoveryRaw);
    NetGatewayHostTransportFactory factory(cfg.m_service);
    std::unique_ptr<NetGatewayTransport> gateway(factory.create());
    PeerTestListener listener;
    NetInterfaceLinkOptions options = {};
    options.m_version = 3;
    options.m_role = NI_LINK_CLIENT;
    uint64_t peer = 0;
    CHECK_TEST(gateway && gateway->connect("127.0.0.1", 9101, options, &listener, peer) == NI_OK && peer == 1);
    CHECK_TEST(service == 1 && serviceRaw == 1 && !discovery && !discoveryRaw);
    gateway->shutdown();
    std::vector<NetGatewayAddress> seeds;
    CHECK_TEST(ParseTestSeeds("127.0.0.1:9102", seeds) == NI_OK);
    NetGatewayDiscovery probe(cfg.m_discovery);
    CHECK_TEST(probe.start(seeds) == NI_OK && probe.poll());
    CHECK_TEST(discovery == 1 && discoveryRaw == 1 && service == 1 && serviceRaw == 1);
    probe.shutdown();
    CHECK_TEST(probe.stopped());
    return 0;
}

/** 用同一实例更正参数，核对被拒绝的启动未接管宿主，修正后的启动真正到达宿主
@return 三种运输均满足端口边界和重试约定时返回0
*/
static int EndpointRetry()
{
    CHECK_TEST(InstallWire() == NI_OK && !activeWire);
    static const uint8_t ca[] = TEST_CA_CRT_RSA_SHA256_PEM;
    static const uint8_t cert[] = TEST_SRV_CRT_RSA_SHA256_PEM;
    static const uint8_t key[] = TEST_SRV_KEY_RSA_PEM;
    NetInterfaceTlsOptions tls = {};
    tls.authority = ca;
    tls.authoritySize = sizeof(ca);
    tls.hostname = "localhost";
    NetInterfaceTlsOptions server = {};
    server.server = 1;
    server.certificate = cert;
    server.certificateSize = sizeof(cert);
    server.privateKey = key;
    server.privateKeySize = sizeof(key);
    PeerTestListener listener;
    uint64_t id = 99;
    {
        NetTransport direct;
        CHECK_TEST(direct.listen("", 0, server, &listener, 1) == NI_INVALID_ARGUMENT && !activeWire);
        CHECK_TEST(direct.connect("", 9001, tls, &listener, id) == NI_INVALID_ARGUMENT && !id && !activeWire);
        CHECK_TEST(direct.listen("127.0.0.1", 65536, server, &listener, 1) == NI_INVALID_ARGUMENT);
        CHECK_TEST(direct.connect("127.0.0.1", 0, tls, &listener, id) == NI_INVALID_ARGUMENT && id == 0);
        LockFreeHashMapPoolConfig empty;
        empty.setInitialCapacity(0);
        empty.setGrowPolicy([](const LockFreeHashMapGrowContext&) -> size_t { return 0; });
        CHECK_TEST(!activeWire && direct.configurePeerPool(empty) == NI_OK);
        CHECK_TEST(direct.connect("127.0.0.1", 9001, tls, &listener, id) == NI_LIMIT_EXCEEDED && id == 0);
        CHECK_TEST(!activeWire && direct.configurePeerPool(LockFreeHashMapPoolConfig()) == NI_OK);
        CHECK_TEST(direct.listen("127.0.0.1", 0, server, &listener, 1) == NI_OK && direct.localPort() == 40123 && direct.poll());
    }
    {
        NetLinkTransport link;
        NetInterfaceLinkOptions options = {};
        options.m_version = 3;
        options.m_role = NI_LINK_CLIENT;
        id = 99;
        CHECK_TEST(link.connect("", 9001, options, &listener, id) == NI_INVALID_ARGUMENT && !id && !activeWire);
        CHECK_TEST(link.connect(nullptr, 9001, options, &listener, id) == NI_INVALID_ARGUMENT && id == 0);
        CHECK_TEST(!activeWire && link.configurePeerPool(LockFreeHashMapPoolConfig()) == NI_OK);
        CHECK_TEST(!listener.readyCount && !listener.closeCount);
        CHECK_TEST(link.connect("127.0.0.1", 65535, options, &listener, id) == NI_OK && id && activeWire);
        CHECK_TEST(!listener.closeCount);
        link.shutdown();
        CHECK_TEST(listener.closeCount == 1 && listener.closeReason == NI_OK);
    }
    {
        NetRelayTransport relay;
        CHECK_TEST(relay.listen("", 0, &listener, 1) == NI_INVALID_ARGUMENT && !activeWire);
        CHECK_TEST(relay.connect("", 9001, &listener, id) == NI_INVALID_ARGUMENT && !id && !activeWire);
        CHECK_TEST(relay.listen("127.0.0.1", 9001, &listener, 0) == NI_INVALID_ARGUMENT);
        id = 99;
        CHECK_TEST(relay.connect("127.0.0.1", 65536, &listener, id) == NI_INVALID_ARGUMENT && id == 0);
        CHECK_TEST(!activeWire && relay.configurePools(NetRelayPoolConfig()) == NI_OK);
        CHECK_TEST(relay.listen("127.0.0.1", 65535, &listener, 1) == NI_OK && relay.localPort() == 65535 && relay.poll());
    }
    CHECK_TEST(!activeWire && listener.readyCount == 0 && listener.closeCount == 1);
    return 0;
}

/** 在 TCP 建连和首次 TLS 推进之间交付宿主终态，核对本地和远端关闭原因
@param [in] eventBudget 每轮可以派发的原始宿主事件数
@return 各种关闭均只通知一次且保留预期原因时返回0
*/
static int CheckEarlyEof(uint32_t eventBudget)
{
    CHECK_TEST(InstallWire() == NI_OK);
    static const uint8_t ca[] = TEST_CA_CRT_RSA_SHA256_PEM;
    NetInterfaceTlsOptions tls = {};
    tls.authority = ca;
    tls.authoritySize = sizeof(ca);
    tls.hostname = "localhost";
    for (bool connected : {false, true})
    {
        for (int32_t reason : {0, -83})
        {
            for (bool local : {false, true})
            {
                PeerTestListener listener;
                NetDirectTransportConfig config;
                config.m_host.m_eventBudget = eventBudget;
                NetTransport transport(config);
                uint64_t id = 0;
                CHECK_TEST(transport.connect("127.0.0.1", 9123, tls, &listener, id) == NI_OK && activeWire && id);
                if (connected)
                {
                    activeWire->connected(activeWire->observer, id);
                }
                if (local)
                {
                    transport.close(id, reason);
                }
                else
                {
                    activeWire->live = false;
                    activeWire->closed(activeWire->observer, id, reason);
                }
                transport.poll();
                transport.poll();
                CHECK_TEST(listener.closeCount == 1 && listener.readyCount == 0 && transport.peerCount() == 0);
                const int32_t expected = local ? reason : (reason ? NI_INVALID_STATE : NET_INTERFACE_TLS_TRUNCATED);
                CHECK_TEST(listener.closeReason == expected && listener.closeDetail == (local ? 0 : reason));
                CHECK_TEST(activeWire->closes == (local ? 1U : 0U));
            }
        }
    }
    NetInterfaceLinkOptions options = {};
    options.m_version = 3;
    options.m_role = NI_LINK_CLIENT;
    NetLinkTransportConfig config;
    config.m_host.m_eventBudget = eventBudget;
    for (bool connected : {false, true})
    {
        for (bool local : {false, true})
        {
            for (int32_t reason : {0, -83})
            {
                PeerTestListener listener;
                NetLinkTransport link(config);
                uint64_t id = 0;
                CHECK_TEST(link.connect("127.0.0.1", 9123, options, &listener, id) == NI_OK && activeWire && id);
                if (connected) { activeWire->connected(activeWire->observer, id); }
                if (local)
                {
                    link.close(id, reason);
                }
                else
                {
                    activeWire->live = false;
                    activeWire->closed(activeWire->observer, id, reason);
                }
                link.poll();
                link.poll();
                const int32_t expected = local ? reason : (reason ? NI_INVALID_STATE : NI_TRUNCATED);
                CHECK_TEST(listener.closeCount == 1 && listener.closeReason == expected && !listener.readyCount);
                CHECK_TEST(activeWire->closes == (local ? 1U : 0U));
            }
        }
    }
    return 0;
}

/** 比较单事件派发与默认预算下的相同关闭序列
@return 两种配置均保留关闭顺序和原因时返回0
*/
static int EarlyEof()
{
    CHECK_TEST(CheckEarlyEof(1) == 0);
    return CheckEarlyEof(NetHostPoolConfig().m_eventBudget);
}

/** 比较随机接口的参数错误、部分写擦除和边界保护
@return 全部通过为0
*/
static int EntropyBounds()
{
    CHECK_TEST(InstallWire() == NI_OK);
    uint8_t bytes[] = {42,42,42,42};
    CHECK_TEST(NetPeer::randomBytes(nullptr, 0) == NI_OK && NetTransport::randomBytes(nullptr, 1) == NI_INVALID_ARGUMENT);
    CHECK_TEST(NetPeer::randomBytes(bytes + 1, 2) == NI_OK && bytes[0] == 42 && bytes[1] == 7 && bytes[2] == 8 && bytes[3] == 42);
    failEntropy = true;
    const int rc = NetTransport::randomBytes(bytes + 1, 2);
    failEntropy = false;
    CHECK_TEST(rc == NI_INVALID_STATE && bytes[0] == 42 && bytes[1] == 0 && bytes[2] == 0 && bytes[3] == 42);
    return 0;
}

/** 核对拨号期间的同步 connected/EOF 与返回编号；启动拒绝时不能向监听器交付事件
@return 全部通过为0
*/
static int CheckSynchronousDial()
{
    const PeerTestDialCase cases[] = {
        {1, 0, 1, true}, {-1, 0, 1, true}, {1, -1, 1, true}, {-1, -1, 1, true},
        {1, 2, 2, false}, {-1, 2, 1, false}, {1, -2, 1, false}, {2, -1, 2, false},
        {-1, -2, 2, false}, {-1, 1, 1, false}, {1, 0, 2, false}, {-1, 0, 2, false}
    };
    for (const PeerTestDialCase& input : cases)
    {
        PeerTestListener listener;
        NetHostSocket socket;
        // 借用当前宿主回调，在输出拨号编号前依序交付至多两个事件。
        synchronousDial = [&input](PeerTestWire& wire, uint64_t& peer) {
            const int32_t events[] = {input.m_first, input.m_second};
            for (int32_t event : events)
            {
                if (event > 0)
                {
                    wire.connected(wire.observer, static_cast<uint64_t>(event));
                }
                if (event < 0)
                {
                    wire.live = false;
                    wire.closed(wire.observer, static_cast<uint64_t>(-event), 0);
                }
            }
            peer = input.m_returned;
        };
        uint64_t peer = 99;
        const int32_t result = socket.connect("127.0.0.1", 9000, &listener, peer);
        synchronousDial = nullptr;
        CHECK_TEST(listener.connectedCount == 0 && listener.closeCount == 0);
        if (input.m_accepted)
        {
            CHECK_TEST(result == NI_OK && peer == input.m_returned && socket.peerCount() == 1);
            const bool closed = input.m_first < 0 || input.m_second < 0;
            CHECK_TEST(socket.poll() == !closed && listener.connectedCount == (input.m_first > 0 ? 1U : 0U));
            CHECK_TEST(listener.closeCount == (closed ? 1U : 0U) && listener.closeReason == NI_OK);
            CHECK_TEST(socket.peerCount() == (closed ? 0U : 1U) && socket.status() == NI_OK);
        }
        else
        {
            CHECK_TEST(result == NI_PROTOCOL_ERROR && peer == 0 && !socket.poll() && socket.peerCount() == 0);
            CHECK_TEST(listener.connectedCount == 0 && listener.closeCount == 0);
        }
    }
    return 0;
}

/** 在真实宿主输入回调中核对借用正文、大小级增长和停止后回收
@return 全部通过为0
*/
static int HostInput()
{
    CHECK_TEST(InstallWire() == NI_OK);
    CHECK_TEST(CheckSynchronousDial() == 0);
    size_t counts[2] = {};
    NetHostPoolConfig cfg;
    cfg.m_eventBudget = 1;
    ObservePool(cfg.m_peers, counts[0]);
    ObservePool(cfg.m_input.m_classes[1], counts[1]);
    PeerTestListener listener;
    NetHostPoolConfig host;
    host.m_eventBudget = cfg.m_eventBudget;
    NetHostSocket socket(nullptr, host);
    CHECK_TEST(socket.configureInputPool(cfg.m_input) == NI_OK && socket.configurePeerPool(cfg.m_peers) == NI_OK);
    uint64_t id = 0;
    CHECK_TEST(socket.connect("127.0.0.1", 9000, &listener, id) == NI_OK);
    activeWire->connected(activeWire->observer, id);
    CHECK_TEST(socket.poll() && listener.connectedCount == 1);
    CHECK_TEST(socket.pauseRead(id, false) == NI_OK);
    std::vector<uint8_t> original(65);
    for (size_t i = 0; i < original.size(); ++i) { original[i] = static_cast<uint8_t>(i + 33); }
    std::vector<uint8_t> buf = original;
    activeWire->data(activeWire->observer, id, buf.data(), static_cast<uint32_t>(buf.size()));
    activeWire->writable(activeWire->observer, id);
    activeWire->live = false;
    activeWire->closed(activeWire->observer, id, -123);
    // 远端终态先到，再次关闭须保留已接纳事件和最初原因。
    socket.close(id, NI_CANCELLED);
    CHECK_TEST(activeWire->closes == 0);
    std::fill(buf.begin(), buf.end(), 0);
    CHECK_TEST(socket.poll() && counts[0] == 1 && counts[1] == 1 && listener.inputs == 1 && listener.input == original);
    CHECK_TEST(!listener.writableCount && socket.poll() && listener.writableCount == 1);
    CHECK_TEST(!listener.closeCount && !socket.poll() && listener.closeCount == 1 && listener.closeReason == NI_INVALID_STATE && listener.closeDetail == -123 && socket.status() == NI_OK);
    CHECK_TEST(socket.uninit() == NI_OK && !activeWire);
#if defined(__EMSCRIPTEN__)
    CHECK_TEST(netPeerWasmConfigureGateways(nullptr, nullptr, nullptr) < 0);
    const int jsPools = EM_ASM_INT({
        const runtime = new Module.NetPeerRuntime({
            createSocket:function(){}, randomSource:function(){},
            hostEventBudget:1, randomRefillBytes:3, hostBufferPools:{inputBytes:32768, outputBytes:-1}
        });
        let rejected = false;
        try { new Module.NetPeerRuntime({createSocket:function(){},randomSource:function(){},hostEventBudget:0}); }
        catch (error) { rejected = error instanceof RangeError; }
        if (!rejected || runtime.hostEventBudget !== 1) return 0;
        const address = Module._malloc(4);
        if (!address) return 0;
        let refills = 0;
        runtime.refillRandom = () => { ++refills; return Promise.resolve(); };
        runtime.random = new Uint8Array([11,12,13,14,15]);
        const boundary = runtime.copyRandom(address, 2) === 0 && refills === 0 &&
            runtime.copyRandom(address + 2, 1) === 0 && refills === 1 && Module.HEAPU8[address + 2] === 13;
        Module._free(address);
        if (!boundary) return 0;
        const input = runtime.inputPool.acquire(32768);
        const first = runtime.outputPool.acquire(32768);
        const second = runtime.outputPool.acquire(32768);
        if (!input || !first || !second || runtime.inputPool.acquire(1) !== null) return 0;
        input[0] = 99;
        runtime.inputPool.release(input);
        const reused = runtime.inputPool.acquire(32768);
        return reused === input && reused[0] === 0 && first !== second ? 1 : 0;
    });
    CHECK_TEST(jsPools == 1);
#endif
    return 0;
}

PeerTestFileBackend::~PeerTestFileBackend()
{
    if (m_stopCallsOnDestruction != nullptr)
    {
        *m_stopCallsOnDestruction = stopCalls;
    }
}

int32_t PeerTestFileBackend::init()
{ return NI_OK; }

int32_t PeerTestFileBackend::nextRequestId(uint64_t& id)
{ id = ++next; return NI_OK; }

int32_t PeerTestFileBackend::submit(const NetFileIoRequest& request)
{
    if (completion.id) { return NI_LIMIT_EXCEEDED; }
    if (request.operation == NET_FILE_IO_WRITE)
    {
        if (!request.bytes || !request.size || request.flags != 1 || (!cache && (request.offset != written.size() || request.size != 65536))) { return NI_INVALID_ARGUMENT; }
        if (cache) { written.clear(); }
        written.insert(written.end(), request.bytes, request.bytes + request.size);
    }
    completion.id = request.id;
    completion.file = request.file;
    completion.offset = request.offset;
    completion.operation = request.operation;
    completion.result = NI_OK;
    if (request.operation == NET_FILE_IO_WRITE) { completion.length = request.size; }
    if (cache && request.operation == NET_FILE_IO_OPEN_READ)
    {
        completion.result = saved.empty() ? NI_NOT_FOUND : NI_OK;
        completion.file = saved.empty() ? 0 : 76;
        completion.length = saved.size();
    }
    if (cache && request.operation == NET_FILE_IO_READ && !saved.empty())
    {
        if (request.file != 76 || request.offset != 0 || request.size != saved.size())
        {
            completion.result = NI_INVALID_ARGUMENT;
        }
        else
        {
            completion.bytes = saved.data();
            completion.size = request.size;
        }
    }
    if (cache && request.operation == NET_FILE_IO_OPEN_TEMPORARY) { completion.file = 77; }
    return NI_OK;
}

int32_t PeerTestFileBackend::cancel(uint64_t id)
{ return invalidReply ? NI_READY : completion.id == id ? NI_OK : NI_NOT_FOUND; }

int32_t PeerTestFileBackend::peek(NetFileIoResult& out)
{ out = completion; return out.id ? NI_READY : NI_NEED_MORE; }

int32_t PeerTestFileBackend::consume(uint64_t id)
{
    if (invalidReply) { return NI_NEED_MORE; }
    if (!id || id != completion.id) { return NI_INVALID_STATE; }
    if (cache && completion.operation == NET_FILE_IO_PUBLISH && completion.result == NI_OK)
    {
        saved = written;
        ++published;
    }
    completion = NetFileIoResult();
    return NI_OK;
}

void PeerTestFileBackend::stop()
{
    ++stopCalls;
    if (rejectNextStop) { rejectNextStop = false; throw std::runtime_error("stop"); }
    stoppedInsideQuery = stoppedInsideQuery || (m_query && m_query->m_active);
    stopped = true;
}

bool PeerTestFileBackend::done() const
{
    if (m_query && m_query->m_rejectNext) { m_query->m_rejectNext = false; throw std::runtime_error("done"); }
    if (m_query && m_query->m_stopOnQuery)
    {
        NetFileIo* owner = m_query->m_stopOnQuery;
        m_query->m_stopOnQuery = nullptr;
        m_query->m_active = true;
        owner->stop();
        m_query->m_active = false;
    }
    return stopped && !m_deferDone;
}

void PeerTestFileBackend::uninit()
{
    if (rejectNextRelease) { rejectNextRelease = false; throw std::runtime_error("uninit"); }
    if (m_releases) { ++*m_releases; }
    stopped = true;
}

static PeerTestFileBackend* fileBackend = nullptr;
// 一次性模拟宿主工厂分配失败，随后允许同一门面重新初始化。
static bool rejectNextFileFactory = false;

/** 为本进程安装可控文件后端，所有用例独立创建并释放实例
@return 安装成功为0
*/
static int InstallFileBackend()
{
    static bool installed = false;
    if (!installed)
    {
        CHECK_TEST(NetFileIo::configureBackend([]() -> NetFileIoBackend* {
            if (rejectNextFileFactory)
            {
                rejectNextFileFactory = false;
                fileBackend = nullptr;
                return nullptr;
            }
            fileBackend = new (std::nothrow) PeerTestFileBackend;
            return fileBackend;
        }) == NI_OK);
        installed = true;
    }
    return 0;
}

/** 核对合并写跨越宿主单次I/O上限时的实际内容、ACK时机及超时
@return 全部通过为0
*/
static int FileBatch()
{
    PeerFileQueryProbe query;
    CHECK_TEST(InstallFileBackend() == 0);
    unsigned stopCallsOnDestruction = 0;
    unsigned releases = 0;
    {
        NetFileIo retiring;
        CHECK_TEST(retiring.init() == NI_OK && fileBackend);
        fileBackend->m_deferDone = true;
        fileBackend->m_stopCallsOnDestruction = &stopCallsOnDestruction;
        fileBackend->m_releases = &releases;
        retiring.uninit();
        CHECK_TEST(!retiring.released() && !retiring.done() && retiring.status() == NI_INVALID_STATE &&
            fileBackend->stopCalls == 1 && releases == 0 && stopCallsOnDestruction == 0);
    }
    CHECK_TEST(stopCallsOnDestruction == 1 && releases == 0);
    NetFileIo io;
    CHECK_TEST(io.init() == NI_OK && fileBackend);
    fileBackend->invalidReply = true;
    CHECK_TEST(io.cancel(1) == NI_INVALID_STATE && io.status() == NI_INVALID_STATE);
    CHECK_TEST(io.consume(1) == NI_INVALID_STATE && io.status() == NI_INVALID_STATE);
    fileBackend->invalidReply = false;
    NetInterfaceFileInfo info = {};
    info.direction = 1; info.state = NI_FILE_TRANSFERRING;
    info.chunk = 65536; info.window = 2; info.total = 131072;
    info.stream = 61; info.expiresUtc = 3600;
    info.transfer[0] = 1; info.token[0] = 2; info.hash[0] = 3;
#if defined(__EMSCRIPTEN__)
    uint8_t metadata[160] = {};
    CHECK_TEST(NetInterfaceFile::encodeInfo(info, metadata, sizeof(metadata)) == NI_OK);
    const int jsFile = EM_ASM_INT({ return (function() {
        const start = Number($0);
        const metadata = Module.HEAPU8.slice(start, start + 160);
        const client = {runtime:{module:Module},options:{clock:function(){return 100;}}};
        const io = {read:function(){},write:function(){},flush:function(){},close:function(){},commit:function(){},
            maxWriteBytes:131072,progressTimeoutMs:1000,pollIntervalMs:17};
        for (const product of [0, -1, 1.5, 4294967303, Number.NaN]) {
            try {
                const unexpected = new Module.NetFileTransfer(client, metadata, product, false, io);
                unexpected.destroy();
                return 0;
            } catch (error) { if (!(error instanceof RangeError)) return 0; }
        }
        const file = new Module.NetFileTransfer(client, metadata, 4294967295, false, io);
        let delay = 0;
        const timer = globalThis.setTimeout;
        globalThis.setTimeout = (work, ms) => { delay = ms; return 0; };
        try { file.schedule(); } finally { globalThis.setTimeout = timer; }
        const ok = file.handle && file.batchWrites && file.stream[0] === 61 && delay === 17;
        file.destroy();
        for (const limit of [0, -2, 1.5, Number.NaN]) {
            io.maxWriteBytes = limit;
            try {
                const unexpected = new Module.NetFileTransfer(client, metadata, 7, false, io);
                unexpected.destroy();
                return 0;
            } catch (error) { if (!(error instanceof RangeError)) return 0; }
        }
        io.maxWriteBytes = -1;
        const unlimited = new Module.NetFileTransfer(client, metadata, 7, false, io);
        const window = unlimited.batchWrites;
        unlimited.destroy();
        if (!window) return 0;
        io.progressTimeoutMs = 999;
        try { new Module.NetFileTransfer(client, metadata, 7, false, io); return 0; }
        catch (error) { return ok && file.done() && error instanceof RangeError ? 1 : 0; }
    })(); }, metadata);
    CHECK_TEST(jsFile == 1);
#endif
    NetFileTransfer receiver;
    CHECK_TEST(receiver.setWriteBatchChunks(0) == NI_INVALID_ARGUMENT && receiver.setWriteBatchChunks(9) == NI_INVALID_ARGUMENT);
    CHECK_TEST(receiver.setWriteBatchChunks(3) == NI_OK && receiver.start(io, 1, info, 7, false, nullptr, nullptr, 100) == NI_INVALID_ARGUMENT);
    CHECK_TEST(receiver.setWriteBatchChunks(2) == NI_OK && receiver.setProgressTimeout(999) == NI_INVALID_ARGUMENT &&
        receiver.setProgressTimeout(60001) == NI_INVALID_ARGUMENT && receiver.setProgressTimeout(1000) == NI_OK);
    CHECK_TEST(receiver.start(io, 1, info, 7, false, nullptr, nullptr, 100) == NI_OK);
    CHECK_TEST(receiver.setWriteBatchChunks(1) == NI_INVALID_STATE && receiver.setProgressTimeout(1500) == NI_INVALID_STATE);
    NetInterfaceFileFlow source(info, 7, true, 100);
    std::vector<uint8_t> content(131072);
    for (size_t i = 0; i < content.size(); ++i) { content[i] = static_cast<uint8_t>(i % 251); }
    for (uint64_t i = 0; i < 2; ++i)
    {
        uint64_t offset = 0;
        uint32_t size = 0;
        NetInterfaceFrame data = {};
        CHECK_TEST(source.readRequest(offset, size) == NI_READY && offset == i * 65536 && size == 65536);
        CHECK_TEST(source.produced(offset, content.data() + static_cast<size_t>(offset), size) == NI_OK &&
            source.peekOutput(data) == NI_READY && receiver.accept(data) == NI_OK && source.consumeOutput() == NI_OK);
        CHECK_TEST(receiver.poll(101 + i) == NI_OK && fileBackend->written.size() == i * 65536);
    }
    NetInterfaceFrame ack = {};
    for (uint64_t i = 0; i < 2; ++i)
    {
        NetFileIoResult result = {};
        CHECK_TEST(receiver.peekOutput(ack) == NI_NEED_MORE && io.peek(result) == NI_READY &&
            result.offset == i * 65536 && receiver.onIoResult(result) && io.consume(result.id) == NI_OK);
        CHECK_TEST(receiver.poll(103 + i) == NI_OK);
    }
    CHECK_TEST(fileBackend->written == content && receiver.peekOutput(ack) == NI_READY && ack.m_kind == NI_DATA_ACK);
    CHECK_TEST(receiver.consumeOutput() == NI_OK);
    CHECK_TEST(receiver.poll(1102) == NI_OK && receiver.poll(1103) == NI_TIMEOUT);
    NetFileIoResult closed = {};
    fileBackend->completion.result = NI_INVALID_STATE;
    CHECK_TEST(io.peek(closed) == NI_READY && closed.operation == NET_FILE_IO_CLOSE && receiver.onIoResult(closed) &&
        io.consume(closed.id) == NI_OK && receiver.poll(1104) == NI_TIMEOUT && receiver.done());
    NetFileIoRequest pending = {};
    CHECK_TEST(io.nextRequestId(pending.id) == NI_OK);
    pending.operation = NET_FILE_IO_SIZE;
    pending.file = 1;
    CHECK_TEST(io.submit(pending) == NI_OK);
    NetFileTransfer candidate;
    fileBackend->m_query = &query;
    query.m_rejectNext = true;
    CHECK_TEST(candidate.start(io, 2, info, 7, false, nullptr, nullptr, 100) == NI_INVALID_ARGUMENT &&
        candidate.done() && io.status() == NI_INVALID_STATE && io.accepting());
    query.m_stopOnQuery = &io;
    CHECK_TEST(candidate.start(io, 2, info, 7, false, nullptr, nullptr, 100) == NI_INVALID_ARGUMENT &&
        candidate.done() && !fileBackend->stoppedInsideQuery && fileBackend->stopCalls == 1);
    io.stop();
    CHECK_TEST(io.cancel(pending.id) == NI_OK && io.peek(closed) == NI_READY && closed.id == pending.id &&
        io.consume(closed.id) == NI_OK && io.done() && fileBackend->stopCalls == 1);
    CHECK_TEST(!io.released());
    io.uninit();
    CHECK_TEST(io.released() && io.init() == NI_OK && !io.released());
    fileBackend->rejectNextStop = true;
    io.stop();
    CHECK_TEST(io.status() == NI_INVALID_STATE && fileBackend->stopCalls == 1);
    CHECK_TEST(candidate.start(io, 2, info, 7, false, nullptr, nullptr, 100) == NI_INVALID_ARGUMENT && candidate.done());
    io.stop();
    CHECK_TEST(fileBackend->stopCalls == 2 && io.done());
    io.stop();
    CHECK_TEST(fileBackend->stopCalls == 2);
    io.uninit();
    CHECK_TEST(io.init() == NI_OK && candidate.start(io, 2, info, 7, false, nullptr, nullptr, 100) == NI_OK);
    candidate.cancel();
    CHECK_TEST(candidate.poll(101) == NI_CANCELLED && io.peek(closed) == NI_READY && closed.operation == NET_FILE_IO_CLOSE);
    CHECK_TEST(candidate.onIoResult(closed) && io.consume(closed.id) == NI_OK && candidate.poll(102) == NI_CANCELLED && candidate.done());
    return 0;
}

/** 逐个将正数预算置零，通过公开准备入口检查拒绝和恢复。
@return 全部通过为0
*/
static int NumericOptions()
{
    NetPeer& api = NetPeer::instance();
    NetPeerSenderState* client = nullptr;
    CHECK_TEST(api.createClient(client) == NI_OK && client);
    const auto release = [&](NetPeerSenderState* state) { api.destroyClient(state); };
    std::unique_ptr<NetPeerSenderState, decltype(release)> owner(client, release);
    NetPeerConfig cfg;
    uint32_t* direct[] = {&cfg.m_direct.m_host.m_eventBudget, &cfg.m_direct.m_outputBudget, &cfg.m_direct.m_frameBudget, &cfg.m_direct.m_drainTimeoutMs,
        &cfg.m_direct.m_payloadLimit, &cfg.m_direct.m_connection.m_progressStepBudget, &cfg.m_discovery.m_retryMs,
        &cfg.m_discovery.m_responseMs, &cfg.m_discovery.m_refreshMs};
    std::vector<uint32_t*> fields(direct, direct + sizeof(direct) / sizeof(direct[0]));
    NetLinkTransportConfig* paths[] = {&cfg.m_service, &cfg.m_discovery.m_transport};
    for (auto path : paths)
    {
        uint32_t* link[] = {&path->m_host.m_eventBudget, &path->m_stepBudget, &path->m_backpressureMs, &path->m_relay.m_stepBudget,
            &path->m_relay.m_startupMs, &path->m_relay.m_backpressureMs, &path->m_link.m_pingIntervalMs,
            &path->m_link.m_pongTimeoutMs, &path->m_link.m_drainTimeoutMs, &path->m_link.m_controlStepBudget,
            &path->m_link.m_controlConnection.m_progressFrameBudget};
        fields.insert(fields.end(), link, link + sizeof(link) / sizeof(link[0]));
    }
    for (auto field : fields)
    {
        const uint32_t saved = *field;
        *field = 0;
        CHECK_TEST(api.prepareGateway(client, cfg) == NI_INVALID_ARGUMENT);
        *field = saved;
    }
    NetConnectionPoolConfig* codecs[] = {&cfg.m_direct.m_connection, &cfg.m_service.m_link.m_controlConnection,
        &cfg.m_discovery.m_transport.m_link.m_controlConnection};
    for (auto codec : codecs)
    {
        const NetConnectionPoolConfig saved = *codec;
        int32_t* signedLimits[] = {&codec->m_decoderRetentionBytes, &codec->m_controlPayloadLimit};
        for (auto limit : signedLimits)
        {
            *limit = -2;
            CHECK_TEST(api.prepareGateway(client, cfg) == NI_INVALID_ARGUMENT);
            *limit = -1;
        }
        codec->m_compressionLevel = -1;
        CHECK_TEST(api.prepareGateway(client, cfg) == NI_INVALID_ARGUMENT);
        codec->m_compressionLevel = 10;
        CHECK_TEST(api.prepareGateway(client, cfg) == NI_INVALID_ARGUMENT);
        codec->m_compressionLevel = 9;
        uint32_t* budgets[] = {&codec->m_controlBurst, &codec->m_progressFrameBudget, &codec->m_progressStepBudget};
        for (auto budget : budgets)
        {
            *budget = 0;
            CHECK_TEST(api.prepareGateway(client, cfg) == NI_INVALID_ARGUMENT);
            *budget = 1;
        }
        CHECK_TEST(cfg.valid());
        *codec = saved;
    }
    cfg.m_direct.m_payloadLimit = NET_INTERFACE_DEFAULT_PAYLOAD_LIMIT + 1;
    CHECK_TEST(api.prepareGateway(client, cfg) == NI_INVALID_ARGUMENT);
    cfg.m_direct.m_payloadLimit = NET_INTERFACE_DEFAULT_PAYLOAD_LIMIT;
    cfg.m_discovery.m_backoffMs = cfg.m_discovery.m_jitterMs = cfg.m_discovery.m_cacheWriteMs = 0;
    CHECK_TEST(api.prepareGateway(client, cfg) == NI_OK);
    return 0;
}

PeerConfigChannel::PeerConfigChannel(NetInterfaceHandshake& server) :
m_server(server)
{}

int32_t PeerConfigChannel::configureConnectionPools(const NetConnectionPoolConfig& pools)
{
    m_received = pools;
    return NI_OK;
}

int32_t PeerConfigChannel::configureCompression(uint64_t peer, const uint32_t* types, uint32_t count, bool negotiated)
{
    if (peer != 1 || !negotiated || !types || count != 2) { return NI_INVALID_ARGUMENT; }
    m_types.assign(types, types + count);
    return NI_OK;
}

int32_t PeerConfigChannel::connect(const char*, uint32_t, const NetInterfaceTlsOptions&, NetTransportListener* listener, uint64_t& peer)
{
    peer = 1;
    m_listener = listener;
    return NI_OK;
}

bool PeerConfigChannel::poll()
{
    if (!m_listener) { return false; }
    if (!m_announced) { m_announced = true; m_listener->onReady(1); }
    NetInterfaceFrame frame = {};
    if (m_listener && m_server.peekOutput(frame) == NI_READY && m_listener->onFrame(1, frame))
    {
        m_server.consumeOutput();
    }
    return m_listener != nullptr;
}

int32_t PeerConfigChannel::send(uint64_t, const NetInterfaceFrame& frame)
{
    if (frame.m_kind == NI_HELLO) { return m_server.accept(frame, 1) < 0 ? NI_PROTOCOL_ERROR : NI_OK; }
    if (frame.m_kind == NI_REQUEST)
    {
        NetInterfaceRpcView rpc = {};
        if (NetInterfaceRpc::parse(frame, rpc) != NI_OK || rpc.m_size != m_expectedRpc.size() ||
            (rpc.m_size && (!rpc.m_data || std::memcmp(rpc.m_data, m_expectedRpc.data(), rpc.m_size))))
        {
            return NI_PROTOCOL_ERROR;
        }
        ++m_requests;
    }
    if (frame.m_kind == NI_DATA)
    {
        if (frame.m_requestId != m_dataFrames + 1 || frame.m_payloadSize != m_dataSize ||
            !frame.m_payload || frame.m_payload[0] != frame.m_requestId)
        {
            return NI_PROTOCOL_ERROR;
        }
        for (uint32_t i = 1; i < frame.m_payloadSize; ++i)
        {
            if (frame.m_payload[i] != static_cast<uint8_t>(3 * i + 7))
            {
                return NI_PROTOCOL_ERROR;
            }
        }
        ++m_dataOffers;
        if (m_blockData) { return NI_LIMIT_EXCEEDED; }
        ++m_dataFrames;
    }
    if (frame.m_kind == NI_PING)
    {
        if (!frame.m_requestId || frame.m_productId || frame.m_messageType || frame.m_payloadSize)
        {
            return NI_PROTOCOL_ERROR;
        }
        ++m_pings;
        if (!m_firstPingMs)
        {
            m_firstPingMs = NetInterface::monotonicMilliseconds();
        }
    }
    return NI_OK;
}

int32_t PeerConfigChannel::outputState(uint64_t peer) const
{ return peer == 1 ? NI_OK : NI_INVALID_ARGUMENT; }

void PeerConfigChannel::close(uint64_t, int32_t)
{ m_listener = nullptr; }

void PeerConfigChannel::shutdown()
{ m_listener = nullptr; }

PeerConfiguredClient::PeerConfiguredClient(const std::function<int32_t(NetClientSession&, NetSenderDispatch&)>& setup)
{
    NetPeerClientCallbacks callbacks = {};
    callbacks.structSize = sizeof(callbacks);
    callbacks.version = 1;
    callbacks.context = this;
    // p 借用本测试对象；只保存正式入口交付的值，不重入会话。
    callbacks.ready = [](void* p, uint64_t generation, uint32_t requests, uint32_t chunk)
    {
        PeerConfiguredClient& client = *static_cast<PeerConfiguredClient*>(p);
        ++client.m_ready;
        client.m_valid = client.m_valid && generation != 0 && requests == 8 && chunk == 16384;
    };
    // 超时正文必须为空；取消仅用于释放已发送的缓存验证请求。
    callbacks.result = [](void* p, uint64_t generation, uint64_t id, uint32_t product, uint32_t type,
        uint32_t status, const uint8_t* bytes, uint32_t size)
    {
        PeerConfiguredClient& client = *static_cast<PeerConfiguredClient*>(p);
        client.m_valid = client.m_valid && generation != 0 && id != 0 && product == 7 && type == 17 &&
            size == 0 && bytes == nullptr && (status == NI_RPC_TIMEOUT || status == NI_RPC_CANCELLED);
        if (status == NI_RPC_TIMEOUT)
        {
            client.m_timeouts.push_back(id);
        }
    };
    // 附加服务测试可登记真实帧观察者；未登记时收到事件即为测试失败。
    callbacks.frame = [](void* p, uint64_t generation, const NetInterfaceFrame* frame) -> int32_t
    {
        PeerConfiguredClient& client = *static_cast<PeerConfiguredClient*>(p);
        if (!generation || !frame || !client.m_onFrame)
        {
            client.m_valid = false;
            return 1;
        }
        return client.m_onFrame(*frame) ? 1 : 0;
    };
    // 记录正式关闭原因，句柄释放前本对象始终有效。
    callbacks.closed = [](void* p, uint64_t generation, int32_t reason, int32_t)
    {
        PeerConfiguredClient& client = *static_cast<PeerConfiguredClient*>(p);
        client.m_valid = client.m_valid && generation != 0;
        ++client.m_closed;
        client.m_closeReason = reason;
        client.m_closedAt = NetInterface::monotonicMilliseconds();
    };
    NetPeerConfig config;
    config.m_context = const_cast<std::function<int32_t(NetClientSession&, NetSenderDispatch&)>*>(&setup);
    // context 只借用至 clientCreate 返回；将同一真实会话/派发器交给场景配置。
    config.m_configure = [](void* context, NetClientSession& session, NetSenderDispatch& dispatch) -> int32_t
    {
        return (*static_cast<const std::function<int32_t(NetClientSession&, NetSenderDispatch&)>*>(context))(session, dispatch);
    };
    m_created = NetPeer::instance().clientCreate(&callbacks, &m_handle, config);
}

PeerConfiguredClient::~PeerConfiguredClient()
{
    const int32_t result = release();
    if (result != NI_OK)
    {
        std::fprintf(stderr, "Configured client cleanup failed: %d\n", static_cast<int>(result));
    }
}

int32_t PeerConfiguredClient::release()
{
    if (!m_handle)
    {
        return NI_OK;
    }
    NetPeer& api = NetPeer::instance();
    api.clientClose(m_handle, NI_CANCELLED);
    int32_t result = NI_INVALID_STATE;
    for (unsigned step = 0; step < 16 && result == NI_INVALID_STATE; ++step)
    {
        api.clientPoll(m_handle);
        result = api.clientDestroy(m_handle);
    }
    if (result == NI_OK)
    {
        m_handle = nullptr;
    }
    return result;
}

PeerServiceTransport::PeerServiceTransport(PeerServiceProbe& probe) : m_probe(probe)
{

}

int32_t PeerServiceTransport::connect(const char*, uint32_t, const NetInterfaceLinkOptions&,
    NetLinkTransportListener* listener, uint64_t& peer)
{
    peer = 0;
    if (m_probe.m_refuseConnect)
    {
        return NI_PROTOCOL_ERROR;
    }
    m_listener = listener;
    peer = 1;
    return NI_OK;
}

bool PeerServiceTransport::poll()
{
    if (!m_listener)
    {
        return false;
    }
    if (!m_announced)
    {
        NetInterfaceLinkPeer identity = {};
        identity.m_role = NI_LINK_NODE;
        m_announced = true;
        m_listener->onLinkReady(1, identity);
    }
    else if (!m_response.empty())
    {
        const NetInterfaceFrame response = {0, 1, NI_GATEWAY_OPEN, NI_RESPONSE,
            static_cast<uint32_t>(m_response.size()), reinterpret_cast<const uint8_t*>(m_response.data())};
        m_listener->onLinkControl(1, response);
        m_response.clear();
    }
    else if (m_probe.m_firstCiphertext && !m_probe.m_inputSent)
    {
        // 两条记录之间不推进 TLS：仅小输入队列留下本地后缀并拒绝第二条。
        const uint8_t prefix[258] = {22, 3, 3, 2, 0};
        const uint8_t tail = 0;
        const NetInterfaceTunnelRecord first = {NI_TUNNEL_BYTES, 7, 1, prefix, sizeof(prefix)};
        const NetInterfaceTunnelRecord second = {NI_TUNNEL_BYTES, 7, 1, &tail, 1};
        m_probe.m_inputSent = m_listener->onLinkRecord(1, first);
        if (m_probe.m_inputSent)
        {
            m_probe.m_tailAccepted = m_listener->onLinkRecord(1, second);
        }
    }
    return m_listener != nullptr;
}

/** 在测试模块分配服务回复正文，序列化方只借用目标字节区
@param [in] message 非空的服务控制回复
@param [out] bytes 回传给测试对端的协议正文
@return 是否完整写入非空回复
*/
static bool SerializeTestReply(const ProtoMessage& message, std::string& bytes)
{
    size_t size = 0;
    if (!message.serializedSize(size) || size == 0)
    {
        return false;
    }
    bytes.resize(size);
    size_t written = 0;
    return message.serializeTo(&bytes[0], size, written) && written == size;
}

int32_t PeerServiceTransport::sendControl(uint64_t peer, const NetInterfaceFrame& frame)
{
    if (peer == 1 && frame.m_productId == 0 && frame.m_kind == NI_EVENT &&
        frame.m_messageType == NI_GATEWAY_CREDIT && frame.m_requestId == 0)
    {
        ProtoMessage credit;
        if (!frame.m_payload || !credit.from(frame.m_payload, frame.m_payloadSize))
        {
            return NI_PROTOCOL_ERROR;
        }
        const Variant* value = credit.findValue(NI_GATEWAY_CREDIT_BYTES);
        if (!value || value->type() != Variant::INT32 || value->size() != 4)
        {
            return NI_PROTOCOL_ERROR;
        }
        uint32_t amount = 0;
        for (unsigned i = 0; i < 4; ++i)
        {
            amount |= static_cast<uint32_t>(static_cast<uint8_t>(value->data()[i])) << (8 * i);
        }
        if (!amount || amount > 259)
        {
            return NI_PROTOCOL_ERROR;
        }
        m_probe.m_creditBytes += amount;
        return NI_OK;
    }
    if (peer != 1 || frame.m_productId || frame.m_kind != NI_REQUEST ||
        frame.m_messageType != NI_GATEWAY_OPEN || frame.m_requestId != 1 || !frame.m_payload)
    {
        return NI_PROTOCOL_ERROR;
    }
    ProtoMessage request;
    if (!request.from(frame.m_payload, frame.m_payloadSize))
    {
        return NI_PROTOCOL_ERROR;
    }
    const Variant* window = request.findValue(NI_GATEWAY_WINDOW);
    const Variant* channel = request.findValue(NI_GATEWAY_CHANNEL);
    const uint8_t channelOne[8] = {1};
    if (!window || window->type() != Variant::INT32 || window->size() != 4 ||
        !channel || channel->type() != Variant::UINT64 || channel->size() != 8 ||
        std::memcmp(channel->data(), channelOne, sizeof(channelOne)))
    {
        return NI_PROTOCOL_ERROR;
    }
    m_probe.m_window = 0;
    for (unsigned i = 0; i < 4; ++i)
    {
        m_probe.m_window |= static_cast<uint32_t>(static_cast<uint8_t>(window->data()[i])) << (8 * i);
    }
    ProtoMessage response;
    return response.setBytes(NI_GATEWAY_CHANNEL, channelOne, sizeof(channelOne), Variant::UINT64) &&
        SerializeTestReply(response, m_response) ? NI_OK : NI_NO_MEMORY;
}

int32_t PeerServiceTransport::sendRecord(uint64_t peer, const NetInterfaceTunnelRecord& record)
{
    if (peer != 1 || record.m_kind != NI_TUNNEL_BYTES || record.m_channel != 1 || record.m_product != 7 ||
        !record.m_data || !record.m_size || record.m_data[0] != 22)
    {
        return NI_PROTOCOL_ERROR;
    }
    if (!m_probe.m_firstCiphertext)
    {
        m_probe.m_firstCiphertext = record.m_size;
    }
    return NI_LIMIT_EXCEEDED;
}

void PeerServiceTransport::shutdown()
{
    if (m_listener)
    {
        ++m_probe.m_stopped;
        m_listener = nullptr;
    }
}

int32_t PeerServiceTransport::outputState(uint64_t peer) const
{
    return peer == 1 && m_listener ? NI_OK : NI_INVALID_STATE;
}

NetGatewayTransport* PeerServiceProbe::create()
{
    ++m_created;
    return new (std::nothrow) PeerServiceTransport(*this);
}

PeerPendingDiscovery::PeerPendingDiscovery() : m_active(false), m_choices(0), m_lastChoiceMs(0)
{

}

int32_t PeerPendingDiscovery::start(const std::vector<NetGatewayAddress>& seeds, const std::string&,
    const NetInterfaceTlsOptions&)
{
    m_active = !seeds.empty();
    return m_active ? NI_OK : NI_INVALID_ARGUMENT;
}

bool PeerPendingDiscovery::poll()
{
    return m_active;
}

void PeerPendingDiscovery::shutdown()
{
    m_active = false;
}

bool PeerPendingDiscovery::stopped() const
{
    return !m_active;
}

bool PeerPendingDiscovery::choose(NetClientDiscoveryPoint& endpoint)
{
    ++m_choices;
    endpoint.m_port = 0;
    endpoint.m_address[0] = 0;
    m_lastChoiceMs = NetInterface::monotonicMilliseconds();
    return false;
}

void PeerPendingDiscovery::failed(const NetClientDiscoveryPoint&)
{

}

/** 取得与真实会话相同单位的单调时间，只观察而不修改生产时钟
@return 单调毫秒
*/
static uint64_t PeerOptionNow()
{
    return NetInterface::monotonicMilliseconds();
}

/** 构造本组真实业务握手使用的连接声明
@return 允许八条请求及 16KiB 正文的独立值
*/
static NetPeerClientOptions PeerOptionConnection()
{
    NetPeerClientOptions options = {};
    options.structSize = sizeof(options);
    options.version = 1;
    options.address = "127.0.0.1";
    options.port = 9126;
    options.platform = 1;
    options.capabilities = 3;
    options.maxRequests = 8;
    options.maxChunk = 16384;
    options.handshakeTimeoutMs = 5000;
    options.device[0] = 7;
    return options;
}

/** 创建独立业务握手对端声明
@return 与本组客户端匹配的服务端声明
*/
static NetInterfaceHandshakeOptions PeerOptionHello()
{
    NetInterfaceHandshakeOptions hello = {};
    hello.m_server = 1;
    hello.m_capabilities = 3;
    hello.m_maxRequests = 8;
    hello.m_maxChunk = 16384;
    hello.m_timeoutMs = 5000;
    hello.m_nonce[0] = 19;
    return hello;
}

/** 经过公开连接和推进完成业务握手
@param [in,out] client 当前独占客户端，须已绑定帧通道
@return 正式就绪回调一次且内容正确时返回0
*/
static int ReadyConfiguredClient(PeerConfiguredClient& client)
{
    NetPeer& api = NetPeer::instance();
    const NetPeerClientOptions options = PeerOptionConnection();
    CHECK_TEST(client.m_created == NI_OK && client.m_handle && api.clientConnect(client.m_handle, &options) == NI_OK);
    for (unsigned step = 0; step < 16 && !client.m_ready; ++step)
    {
        CHECK_TEST(api.clientPoll(client.m_handle) >= 0);
    }
    CHECK_TEST(client.m_ready == 1 && client.m_valid && !client.m_closed);
    return 0;
}

/** 比较真实编码缓存容量及公开推进时每轮结算的请求数
@param [in] retain true 保留增长后的缓冲，false 每次编码后释放
@return 正文、缓存与全部八个唯一超时符合各自配置时返回0
*/
static int CheckInitializedStorage(bool retain)
{
    NetInterfaceHandshake remote(PeerOptionHello(), 0);
    PeerConfigChannel channel(remote);
    const uint32_t budget = retain ? 3 : 1;
    // session/dispatch 只在配置回调内借用；通道覆盖客户端生命周期。
    PeerConfiguredClient client([&](NetClientSession& session, NetSenderDispatch&) -> int32_t
    {
        if (session.configurePayloadRetention(retain ? -1 : 0) != NI_OK || session.configureExpiryBudget(budget) != NI_OK)
        {
            return NI_INVALID_STATE;
        }
        return session.bindChannel(&channel);
    });
    CHECK_TEST(ReadyConfiguredClient(client) == 0);
    NetPeer& api = NetPeer::instance();
    channel.m_expectedRpc.resize(8192);
    for (size_t i = 0; i < channel.m_expectedRpc.size(); ++i)
    {
        channel.m_expectedRpc[i] = static_cast<uint8_t>(i * 17 + i / 251);
    }
    uint64_t request = 0;
    CHECK_TEST(api.clientBegin(client.m_handle, 7, 17, 5000, &request) == NI_OK && request &&
        api.clientPost(client.m_handle, request, channel.m_expectedRpc.data(), 8192) == NI_OK);
    for (unsigned step = 0; step < 16 && !channel.m_requests; ++step)
    {
        CHECK_TEST(api.clientPoll(client.m_handle) >= 0);
    }
    // 仅在测试中按本次 SDK 的真实句柄定义只读观察容量，未保留配置回调的引用。
    const uint64_t capacity = client.m_handle->m_session.payloadCapacity();
    CHECK_TEST(channel.m_requests == 1 && (retain ? capacity >= 8208 : capacity == 0));
    CHECK_TEST(api.clientCancel(client.m_handle, request) == NI_OK && api.clientPoll(client.m_handle) >= 0);
    std::vector<uint64_t> ids;
    for (unsigned i = 0; i < 8; ++i)
    {
        CHECK_TEST(api.clientBegin(client.m_handle, 7, 17, 100, &request) == NI_OK && request);
        CHECK_TEST(api.clientPost(client.m_handle, request, channel.m_expectedRpc.data(), 8192) == NI_OK);
        ids.push_back(request);
    }
    for (unsigned step = 0; step < 16 && channel.m_requests != 9; ++step)
    {
        CHECK_TEST(api.clientPoll(client.m_handle) >= 0);
    }
    CHECK_TEST(channel.m_requests == 9 && client.m_timeouts.empty());
    const uint64_t due = PeerOptionNow() + 101;
    while (PeerOptionNow() < due)
    {
        // 只观察已登记请求的共同期限，不能推进并提前消费到期通知。
    }
    CHECK_TEST(api.clientPoll(client.m_handle) >= 0 && client.m_timeouts.size() == 2 * budget);
    for (unsigned i = 0; i < 8 && client.m_timeouts.size() < ids.size(); ++i)
    {
        CHECK_TEST(api.clientPoll(client.m_handle) >= 0);
    }
    CHECK_TEST(client.m_timeouts == ids && client.m_valid && !client.m_closed && client.release() == NI_OK);
    return 0;
}

/** 观察初始化心跳的真实 PING 和两种独立超时关闭
@param [in] receiveFirst true 让总无输入期限先到，false 让 PONG 期限先到
@return PING 和唯一超时关闭均不早于其配置期限时返回0
*/
static int CheckInitializedHeartbeat(bool receiveFirst)
{
    NetInterfaceHandshake remote(PeerOptionHello(), 0);
    PeerConfigChannel channel(remote);
    // heartbeat 是调用方临时值；configure 后的会话必须保有独立设置。
    PeerConfiguredClient client([&](NetClientSession& session, NetSenderDispatch&) -> int32_t
    {
        NetHeartbeatConfig heartbeat;
        heartbeat.m_pingIntervalMs = 20;
        heartbeat.m_pongTimeoutMs = receiveFirst ? 60000 : 80;
        heartbeat.m_receiveTimeoutMs = receiveFirst ? 120 : 60000;
        return session.configureHeartbeat(heartbeat) == NI_OK ? session.bindChannel(&channel) : NI_INVALID_STATE;
    });
    const uint64_t started = PeerOptionNow();
    CHECK_TEST(ReadyConfiguredClient(client) == 0);
    const uint64_t guard = PeerOptionNow() + 3000;
    while (!client.m_closed && PeerOptionNow() < guard)
    {
        CHECK_TEST(NetPeer::instance().clientPoll(client.m_handle) >= 0);
    }
    const uint64_t closed = PeerOptionNow();
    CHECK_TEST(channel.m_pings == 1 && channel.m_firstPingMs + 2 >= started + 20 &&
        client.m_closed == 1 && client.m_closeReason == NI_TIMEOUT && client.m_valid);
    CHECK_TEST(receiveFirst ? closed + 2 >= started + 120 : closed + 2 >= channel.m_firstPingMs + 80);
    CHECK_TEST(client.release() == NI_OK && client.m_closed == 1);
    return 0;
}

/** 从公开入口满队列的可重用槽数观察初始化预取设置
@param [in] batches 一次领取的批数，至少1
@param [in] items 每批项数，本场景采用1或4
@return 背压下只归还对应前缀，随后所有长正文顺序正确时返回0
*/
static int CheckInitializedPrefetch(uint32_t batches, uint32_t items)
{
    NetInterfaceHandshake remote(PeerOptionHello(), 0);
    PeerConfigChannel channel(remote);
    channel.m_dataSize = 65;
    channel.m_blockData = true;
    PeerConfiguredClient client([&](NetClientSession& session, NetSenderDispatch& dispatch) -> int32_t
    {
        NetSenderSubmissionPools pools;
        pools.m_queue.initialCapacity = pools.m_queue.maximumCapacity = 16;
        NetSenderSubmissionOptions options;
        options.m_inputBatchBudget = batches;
        options.m_inputBatchSize = items;
        return dispatch.submission().configurePools(pools, options) == NI_OK ? session.bindChannel(&channel) : NI_INVALID_STATE;
    });
    CHECK_TEST(ReadyConfiguredClient(client) == 0);
    NetPeer& api = NetPeer::instance();
    uint8_t body[65];
    for (uint32_t i = 1; i < sizeof(body); ++i)
    {
        body[i] = static_cast<uint8_t>(3 * i + 7);
    }
    uint32_t original = 0;
    int32_t result = NI_OK;
    for (; original <= 16; ++original)
    {
        body[0] = static_cast<uint8_t>(original + 1);
        const NetInterfaceFrame frame = {7, original + 1, 17, NI_DATA, sizeof(body), body};
        result = api.clientPostData(client.m_handle, &frame);
        if (result != NI_OK)
        {
            break;
        }
    }
    CHECK_TEST(result == NI_LIMIT_EXCEEDED && original >= 8 && original <= 16 && api.clientPoll(client.m_handle) >= 0);
    CHECK_TEST(channel.m_dataOffers != 0 && channel.m_dataFrames == 0);
    uint32_t freed = 0;
    for (; freed <= batches * items; ++freed)
    {
        body[0] = static_cast<uint8_t>(original + freed + 1);
        const NetInterfaceFrame frame = {7, original + freed + 1, 17, NI_DATA, sizeof(body), body};
        result = api.clientPostData(client.m_handle, &frame);
        if (result != NI_OK)
        {
            break;
        }
    }
    CHECK_TEST(result == NI_LIMIT_EXCEEDED && freed == batches * items);
    std::fill(body, body + sizeof(body), 0);
    channel.m_blockData = false;
    for (unsigned step = 0; step < 64 && channel.m_dataFrames < original + freed; ++step)
    {
        CHECK_TEST(api.clientPoll(client.m_handle) >= 0);
    }
    CHECK_TEST(channel.m_dataFrames == original + freed && client.m_valid && !client.m_closed && client.release() == NI_OK);
    return 0;
}

/** 经过 NetPeer 服务连接观察 OPEN 窗口、TLS 输出和总启动期限
@param [in] shortDeadline true 采用80毫秒启动期限，false采用60000毫秒
@return 两组独立值真正到达线格式与TLS，且仅短期限关闭时返回0
*/
static int CheckInitializedService(bool shortDeadline)
{
    PeerServiceProbe probe;
    const uint32_t window = shortDeadline ? 65536 : 131072;
    const uint32_t ciphertext = shortDeadline ? 31 : 67;
    PeerConfiguredClient client([&](NetClientSession& session, NetSenderDispatch&) -> int32_t
    {
        CEncodeDecodeTlsQueueLimits limits;
        if (!limits.set(shortDeadline ? 1 : 512, ciphertext) || session.configureServiceWindow(window) != NI_OK ||
            session.configureServiceTlsQueueLimits(limits) != NI_OK ||
            session.configureServiceTimeouts(shortDeadline ? 80 : 60000, 5000) != NI_OK ||
            session.configureRecovery(60000, 60000, 60000) != NI_OK)
        {
            return NI_INVALID_STATE;
        }
        return session.bindGateway(&probe);
    });
    const NetPeerClientOptions options = PeerOptionConnection();
    NetPeer& api = NetPeer::instance();
    const uint64_t started = PeerOptionNow();
    CHECK_TEST(client.m_created == NI_OK && api.clientConnectService(client.m_handle, 7, &options) == NI_OK);
    for (unsigned step = 0; step < 32 && !probe.m_inputSent && !client.m_closed; ++step)
    {
        CHECK_TEST(api.clientPoll(client.m_handle) >= 0);
    }
    CHECK_TEST(probe.m_created == 1 && probe.m_window == window && probe.m_firstCiphertext == ciphertext && !client.m_ready);
    CHECK_TEST(probe.m_inputSent && probe.m_tailAccepted == !shortDeadline && probe.m_creditBytes == (shortDeadline ? 258U : 259U));
    const uint64_t guard = started + (shortDeadline ? 3000 : 120);
    while (!client.m_closed && PeerOptionNow() < guard)
    {
        CHECK_TEST(api.clientPoll(client.m_handle) >= 0);
    }
    CHECK_TEST(shortDeadline ? client.m_closed == 1 && client.m_closeReason == NI_TIMEOUT && PeerOptionNow() + 2 >= started + 80 :
        client.m_closed == 0);
    CHECK_TEST(client.m_valid && client.release() == NI_OK && probe.m_stopped == 1);
    return 0;
}

/** 没有候选时观察恢复总期限和选择重试间隔
@param [in] timeoutFirst true 让80毫秒总期限先到，否则比较20/60000毫秒重试
@param [in,out] execution 与客户端共享同一 NetInterface 模块的当前执行者
@param [in,out] waited 宿主 wait 写入的真实剩余毫秒预算，每次观察前重置
@return 配置决定的选择次数、资源停止和超时回调正确时返回0
*/
static int CheckInitializedRecovery(bool timeoutFirst, NetExecution& execution, uint32_t& waited)
{
    for (unsigned mode = 0; mode < (timeoutFirst ? 1U : 2U); ++mode)
    {
        PeerServiceProbe factory;
        PeerPendingDiscovery discovery;
        /** 把总期限和选择间隔设置到随后通过公开句柄推进的实际会话
        @param [in,out] session 初始化回调借用的真实会话
        @param [in,out] dispatch 本场景不更改的派发器
        @return 配置及发现绑定均成功返回 NI_OK，否则返回错误
        */
        PeerConfiguredClient client([&](NetClientSession& session, NetSenderDispatch&) -> int32_t
        {
            const uint32_t delay = timeoutFirst || mode ? 60000 : 20;
            // 选择间隔专项的总期限长于60000毫秒，保证观察到的是重试期限。
            return session.configureRecovery(timeoutFirst ? 80 : 120000, delay, delay) == NI_OK ?
                session.bindGateway(&factory, &discovery) : NI_INVALID_STATE;
        });
        NetPeer& api = NetPeer::instance();
        const NetPeerClientOptions options = PeerOptionConnection();
        const uint64_t started = PeerOptionNow();
        CHECK_TEST(client.m_created == NI_OK && api.clientConnectService(client.m_handle, 7, &options) == NI_OK);
        if (!timeoutFirst)
        {
            for (unsigned step = 0; step < 16 && !discovery.m_choices; ++step)
            {
                CHECK_TEST(api.clientPoll(client.m_handle) >= 0);
            }
            CHECK_TEST(discovery.m_choices && !client.m_closed);
            waited = UINT32_MAX;
            execution.begin();
            CHECK_TEST(api.clientPoll(client.m_handle) >= 0 && !client.m_closed && execution.wait(180000) == NI_OK);
            const uint32_t expected = mode ? 60000 : 20;
            CHECK_TEST(waited <= expected && waited + PeerOptionNow() - discovery.m_lastChoiceMs + 2 >= expected);
        }
        const bool waitForRetries = !timeoutFirst && mode == 0;
        const uint64_t guard = started + (timeoutFirst || waitForRetries ? 3000 : 100);
        while (!client.m_closed && PeerOptionNow() < guard && (!waitForRetries || discovery.m_choices < 3))
        {
            CHECK_TEST(api.clientPoll(client.m_handle) >= 0);
        }
        CHECK_TEST(!factory.m_created && !client.m_ready && client.m_valid);
        if (timeoutFirst)
        {
            const uint64_t elapsed = PeerOptionNow() - started;
            // 关闭通知可能发生在本轮发现器推进之后。持续驱动，分别检查超时与发现器停止。
            while (!discovery.stopped() && PeerOptionNow() < guard)
            {
                CHECK_TEST(api.clientPoll(client.m_handle) >= 0);
            }
            if (client.m_closed != 1 || client.m_closeReason != NI_TIMEOUT || discovery.m_choices != 1 ||
                elapsed + 2 < 80 || !discovery.stopped())
            {
                std::fprintf(stderr, "Recovery timeout: closed=%u reason=%d choices=%u elapsed=%u stopped=%d\n",
                    static_cast<unsigned>(client.m_closed), static_cast<int>(client.m_closeReason),
                    discovery.m_choices, static_cast<unsigned>(elapsed), discovery.stopped() ? 1 : 0);
                return 1;
            }
        }
        else
        {
            CHECK_TEST(!client.m_closed && (mode ? discovery.m_choices == 1 : discovery.m_choices >= 3));
        }
        CHECK_TEST(client.release() == NI_OK && discovery.stopped());
    }
    return 0;
}

/** 核对发布完成、取消和最终 ACK 交付的先后顺序、结果及发布次数
@param [in] execution 可空，绑定时额外观察 wait 期限
@param [out] waited 执行宿主记录的等待毫秒，仅 execution 非空时使用
@return 结果、提交次数及终态均正确时返回0
*/
static int CheckFinalFileOutput(NetExecution* execution, uint32_t* waited)
{
    CHECK_TEST(InstallFileBackend() == 0);
    for (unsigned scenario = 0; scenario < 5; ++scenario)
    {
        const bool deliver = scenario == 1 || scenario == 3;
        const bool cancelAtAck = scenario == 2 || scenario == 3;
        NetFileIo io;
        CHECK_TEST(io.init() == NI_OK);
        fileBackend->cache = true;
        NetInterfaceFileInfo info = {};
        info.direction = 1;
        info.state = NI_FILE_TRANSFERRING;
        info.chunk = 1024;
        info.window = 1;
        info.stream = 61;
        info.expiresUtc = 3600;
        info.token[0] = info.transfer[0] = 1;
        const uint8_t empty = 0;
        CHECK_TEST(NetInterface::sha256(&empty, 0, info.hash, sizeof(info.hash)) == NI_OK);
        NetInterfaceFileFlow source(info, 7, true, 100);
        NetFileTransfer receiver;
        CHECK_TEST(receiver.setProgressTimeout(1000) == NI_OK &&
            receiver.start(io, 47, info, 7, false, "test.tmp", "test.file", 100) == NI_OK);
        NetInterfaceFrame frame = {};
        if (execution)
        {
            execution->begin();
            CHECK_TEST(receiver.poll(100) == NI_OK);
        }
        CHECK_TEST(source.peekOutput(frame) == NI_READY && receiver.accept(frame) == NI_OK && source.consumeOutput() == NI_OK);
        if (execution) { CHECK_TEST(execution->wait(60000) == NI_OK && *waited == 0); }
        NetFileIoResult result = {};
        CHECK_TEST(receiver.poll(100) == NI_OK && io.peek(result) == NI_READY && result.operation == NET_FILE_IO_FLUSH);
        if (execution)
        {
            NetFileIoResult unrelated = result;
            ++unrelated.id;
            execution->begin();
            CHECK_TEST(!receiver.onIoResult(unrelated) && execution->wait(60000) == NI_OK && *waited == 60000);
            // FLUSH 在本轮开始前已完成，先 poll 再分派，随后应能立即进入 PUBLISH。
            execution->begin();
            CHECK_TEST(receiver.poll(100) == NI_NEED_MORE);
        }
        CHECK_TEST(receiver.onIoResult(result) && io.consume(result.id) == NI_OK);
        if (execution) { CHECK_TEST(execution->wait(60000) == NI_OK && *waited == 0); }
        CHECK_TEST(receiver.poll(101) == NI_OK && io.peek(result) == NI_READY && result.operation == NET_FILE_IO_PUBLISH &&
            result.result == NI_OK);
        if (scenario == 4)
        {
            // 后端已完成 PUBLISH，完成结果尚未交给本流。先取消协议，再按原结果结算文件。
            receiver.cancel(NI_CANCELLED);
            receiver.cancel(NI_TIMEOUT);
            CHECK_TEST(!receiver.done() && receiver.onIoResult(result) && io.consume(result.id) == NI_OK);
            CHECK_TEST(receiver.poll(102) == NI_CANCELLED && receiver.done());
            int32_t local = NI_OK;
            receiver.snapshot(info, local);
            CHECK_TEST(local == NI_CANCELLED && info.state == NI_FILE_CANCELLED && fileBackend->published == 1 &&
                fileBackend->next == 2 && io.peek(result) == NI_NEED_MORE);
            continue;
        }
        CHECK_TEST(receiver.onIoResult(result) && io.consume(result.id) == NI_OK && fileBackend->published == 1);
        CHECK_TEST(receiver.poll(120) == NI_NEED_MORE && receiver.peekOutput(frame) == NI_READY);
        if (execution) { execution->begin(); }
        const uint64_t before = PeerOptionNow();
        CHECK_TEST(receiver.poll(800) == NI_NEED_MORE && !receiver.done() && receiver.peekOutput(frame) == NI_READY);
        if (execution)
        {
            CHECK_TEST(execution->wait(60000) == NI_OK && *waited <= 320 && *waited + PeerOptionNow() - before + 2 >= 320);
        }
        CHECK_TEST(receiver.poll(1119) == NI_NEED_MORE && receiver.peekOutput(frame) == NI_READY);
        if (deliver) { CHECK_TEST(source.accept(frame) == NI_OK && receiver.consumeOutput() == NI_OK); }
        if (cancelAtAck)
        {
            receiver.cancel(-83);
            receiver.cancel(NI_CANCELLED);
        }
        const int32_t expected = deliver ? NI_READY : (cancelAtAck ? -83 : NI_TIMEOUT);
        CHECK_TEST(receiver.poll(1120) == expected && receiver.done());
        receiver.cancel();
        int32_t local = NI_OK;
        receiver.snapshot(info, local);
        CHECK_TEST(info.state == NI_FILE_COMPLETED && local == expected && receiver.poll(2000) == expected &&
            fileBackend->published == 1 && fileBackend->next == 2 && io.peek(result) == NI_NEED_MORE);
        CHECK_TEST(receiver.peekOutput(frame) == (deliver ? NI_NEED_MORE : expected) && frame.m_payload == nullptr &&
            frame.m_payloadSize == 0 && receiver.consumeOutput() == (deliver ? NI_INVALID_STATE : expected));
    }
    return 0;
}

/** 核对本地客户端的已接纳命令通知执行者，失败和重复关闭保留本轮等待预算
@param [in,out] execution 当前执行者
@param [in,out] waited 等待宿主收到的毫秒数
@return 各命令的调度、实际交付和终态符合预期时返回0
*/
static int CheckClientSubmissionWait(NetExecution& execution, uint32_t& waited)
{
    NetInterfaceHandshake remote(PeerOptionHello(), 0);
    PeerConfigChannel channel(remote);
    PeerConfiguredClient client([&](NetClientSession& session, NetSenderDispatch&) -> int32_t {
        return session.bindChannel(&channel);
    });
    CHECK_TEST(ReadyConfiguredClient(client) == 0);
    NetPeer& api = NetPeer::instance();
    const auto expectsWait = [&](uint32_t limit) -> bool {
        waited = UINT32_MAX;
        return execution.wait(60000) == NI_OK && waited == limit;
    };
    for (unsigned step = 0; step < 16; ++step)
    {
        execution.begin();
        CHECK_TEST(api.clientPoll(client.m_handle) == NI_READY && execution.wait(60000) == NI_OK);
        if (waited != 0) { break; }
    }
    CHECK_TEST(waited > 0);
    const uint8_t body = 73;
    uint64_t request = 0, ticketRequest = 0, rpcTicket = 0, dataTicket = 0;
    CHECK_TEST(api.clientBegin(client.m_handle, 7, 17, 10000, &request) == NI_OK && expectsWait(0));
    execution.begin();
    CHECK_TEST(api.clientPost(client.m_handle, request, &body, 1) == NI_OK && expectsWait(0));
    execution.begin();
    CHECK_TEST(api.clientPost(client.m_handle, request, &body, 1) < 0 && expectsWait(60000));
    CHECK_TEST(api.clientBegin(client.m_handle, 7, 17, 10000, &ticketRequest) == NI_OK && expectsWait(0));
    execution.begin();
    CHECK_TEST(api.clientSendBegin(client.m_handle, ticketRequest, &body, 1, &rpcTicket) == NI_OK && rpcTicket && expectsWait(0));
    uint8_t value = 1;
    NetInterfaceFrame data = {7, 1, 17, NI_DATA, 1, &value};
    execution.begin();
    CHECK_TEST(api.clientPostData(client.m_handle, &data) == NI_OK && expectsWait(0));
    ++data.m_requestId;
    ++value;
    execution.begin();
    CHECK_TEST(api.clientSendDataBegin(client.m_handle, &data, &dataTicket) == NI_OK && dataTicket && expectsWait(0));
    for (unsigned step = 0; step < 16 && channel.m_dataFrames != 2; ++step)
    {
        CHECK_TEST(api.clientPoll(client.m_handle) == NI_READY);
    }
    CHECK_TEST(channel.m_requests == 2 && channel.m_dataFrames == 2 &&
        api.clientSendResult(client.m_handle, rpcTicket) == NI_OK && api.clientSendResult(client.m_handle, dataTicket) == NI_OK);
    execution.begin();
    CHECK_TEST(api.clientCancel(client.m_handle, request) == NI_OK && expectsWait(0));
    execution.begin();
    CHECK_TEST(api.clientCancel(client.m_handle, request) == NI_NOT_FOUND && expectsWait(60000));
    CHECK_TEST(api.clientCancel(client.m_handle, ticketRequest) == NI_OK && expectsWait(0));
    CHECK_TEST(api.clientPoll(client.m_handle) == NI_READY);
    execution.begin();
    CHECK_TEST(api.clientClose(client.m_handle, NI_CANCELLED) == NI_OK && expectsWait(0));
    execution.begin();
    CHECK_TEST(api.clientClose(client.m_handle, NI_PROTOCOL_ERROR) == NI_OK && expectsWait(60000));
    CHECK_TEST(client.release() == NI_OK && client.m_closed == 1 && client.m_closeReason == NI_CANCELLED && client.m_valid);
    return 0;
}

/** 用真实执行者的宿主等待参数核对独立运输、发现及缓存的最早期限
@param [in,out] execution 已绑定的执行者，每次 poll 前重置观察
@param [in,out] waited 宿主 wait 写入的毫秒上限
@return 各阶段均按配置限制等待，且没有把静止轮询变成忙等时返回0
*/
static int CheckTransportWaitBudgets(NetExecution& execution, uint32_t& waited)
{
    // 等待开始前允许测试本身花费时间；两毫秒覆盖单调时钟的毫秒取整。
    const auto matches = [&](uint64_t start, uint32_t limit) -> bool {
        waited = UINT32_MAX;
        return execution.wait(60000) == NI_OK && waited <= limit && waited + (PeerOptionNow() - start) + 2 >= limit;
    };
    for (bool hostFault : {false, true})
    {
        PeerTestListener observed;
        NetHostSocket socket;
        uint64_t id = 0;
        CHECK_TEST(socket.connect("127.0.0.1", 9100, &observed, id) == NI_OK && activeWire);
        activeWire->connected(activeWire->observer, id);
        CHECK_TEST(socket.poll() && observed.connectedCount == 1);
        execution.begin();
        CHECK_TEST(socket.poll() && matches(PeerOptionNow(), 60000));
        if (hostFault)
        {
            // 重复连接通知触发宿主协议错误，尚未入队关闭事件。
            activeWire->connected(activeWire->observer, id);
        }
        else
        {
            // 测试宿主从 close 同步回报 EOF，不会再唤醒等待函数。
            socket.close(id, NI_CANCELLED);
        }
        CHECK_TEST(observed.closeCount == 0 && matches(PeerOptionNow(), 0));
        CHECK_TEST(!socket.poll() && observed.closeCount == 1 &&
            observed.closeReason == (hostFault ? NI_PROTOCOL_ERROR : NI_CANCELLED));
        execution.begin();
        CHECK_TEST(!socket.poll() && observed.closeCount == 1 && matches(PeerOptionNow(), 60000));
    }
    PeerTestListener listener;
    uint64_t peer = 0;
    static const uint8_t ca[] = TEST_CA_CRT_RSA_SHA256_PEM;
    NetInterfaceTlsOptions tls = {};
    tls.authority = ca;
    tls.authoritySize = sizeof(ca);
    tls.hostname = "localhost";
    tls.handshakeTimeoutMs = 4096;
    NetTransport direct;
    execution.begin();
    CHECK_TEST(!direct.poll() && matches(PeerOptionNow(), 60000));
    uint64_t start = PeerOptionNow();
    CHECK_TEST(direct.connect("127.0.0.1", 9100, tls, &listener, peer) == NI_OK);
    CHECK_TEST(listener.connectedCount == 0 && matches(PeerOptionNow(), 0));
    execution.begin();
    CHECK_TEST(direct.poll() && matches(start, tls.handshakeTimeoutMs));
    direct.shutdown();
    NetRelayPoolConfig limits;
    limits.m_startupMs = 4095;
    limits.m_backpressureMs = 1111;
    NetRelayTransport relay(NetHostPoolConfig(), limits);
    execution.begin();
    CHECK_TEST(!relay.poll() && matches(PeerOptionNow(), 60000));
    start = PeerOptionNow();
    CHECK_TEST(relay.connect("127.0.0.1", 9101, &listener, peer) == NI_OK);
    CHECK_TEST(listener.connectedCount == 0 && matches(PeerOptionNow(), 0));
    execution.begin();
    CHECK_TEST(relay.poll() && matches(start, limits.m_startupMs));
    activeWire->connected(activeWire->observer, peer);
    CHECK_TEST(relay.poll() && listener.connectedCount == 1);
    const uint8_t sample = 83;
    const NetInterfaceTunnelRecord record = {NI_TUNNEL_BYTES, 7, 1, &sample, 1};
    uint8_t wire[NET_INTERFACE_TUNNEL_HEADER_SIZE + 1] = {};
    uint32_t size = 0;
    CHECK_TEST(NetInterfaceTunnel::encode(record, wire, sizeof(wire), size) == NI_OK);
    listener.m_rejectRelayRecord = true;
    activeWire->data(activeWire->observer, peer, wire, size);
    start = PeerOptionNow();
    CHECK_TEST(relay.poll() && listener.m_relayOffers == 1);
    execution.begin();
    CHECK_TEST(relay.poll() && listener.m_relayOffers == 2 && matches(start, limits.m_backpressureMs));
    relay.shutdown();

    limits.m_stepBudget = 1;
    PeerTestListener ordered;
    NetRelayTransport batched(NetHostPoolConfig(), limits);
    CHECK_TEST(batched.connect("127.0.0.1", 9101, &ordered, peer) == NI_OK && activeWire);
    activeWire->connected(activeWire->observer, peer);
    CHECK_TEST(batched.poll() && ordered.connectedCount == 1 && !activeWire->paused);
    uint8_t pair[2 * sizeof(wire)] = {};
    CHECK_TEST(size == sizeof(wire));
    std::memcpy(pair, wire, size);
    std::memcpy(pair + size, wire, size);
    pair[2 * size - 1] = 37;
    activeWire->data(activeWire->observer, peer, pair, 2 * size);
    CHECK_TEST(batched.poll() && ordered.m_relayOffers == 0);
    for (unsigned turn = 0; turn < 3; ++turn)
    {
        execution.begin();
        CHECK_TEST(batched.poll() && matches(PeerOptionNow(), 0) && activeWire->paused);
        CHECK_TEST(ordered.m_relayOffers == (turn == 2 ? 2U : 1U) && ordered.m_relayBody.size() == 1 &&
            ordered.m_relayBody[0] == (turn == 2 ? 37 : sample));
    }
    execution.begin();
    CHECK_TEST(batched.poll() && !activeWire->paused && matches(PeerOptionNow(), 60000));
    // 同一执行者刚推进过空闲运输，随后另一项工作才向它提交记录。
    execution.begin();
    CHECK_TEST(batched.poll() && batched.send(peer, record) == NI_OK && matches(PeerOptionNow(), 0));
    CHECK_TEST(batched.send(peer, record) == NI_OK);
    activeWire->m_blockWrites = true;
    execution.begin();
    CHECK_TEST(batched.poll() && !activeWire->writes && matches(PeerOptionNow(), 60000));
    activeWire->m_blockWrites = false;
    activeWire->writable(activeWire->observer, peer);
    CHECK_TEST(batched.send(peer, record) == NI_OK && batched.poll() && activeWire->writes == size);
    execution.begin();
    CHECK_TEST(batched.poll() && activeWire->writes == 2 * size && matches(PeerOptionNow(), 0));
    execution.begin();
    CHECK_TEST(batched.poll() && activeWire->writes == 3 * size && matches(PeerOptionNow(), 60000));
    batched.shutdown();
    NetGatewayDiscoveryConfig config;
    config.m_retryMs = 750;
    config.m_backoffMs = 60000;
    config.m_jitterMs = 0;
    NetGatewayDiscovery discovery(config);
    std::vector<NetGatewayAddress> seeds;
    CHECK_TEST(ParseTestSeeds("127.0.0.1:9102", seeds) == NI_OK);
    execution.begin();
    CHECK_TEST(!discovery.poll() && matches(PeerOptionNow(), 60000));
    CHECK_TEST(discovery.start(seeds) == NI_OK && matches(PeerOptionNow(), 0));
    NetClientDiscoveryPoint candidate = {};
    candidate.m_structSize = sizeof(candidate);
    candidate.m_version = 1;
    CHECK_TEST(discovery.choose(candidate));
    discovery.failed(candidate);
    start = PeerOptionNow();
    execution.begin();
    CHECK_TEST(discovery.poll() && matches(start, config.m_retryMs));
    execution.begin();
    CHECK_TEST(discovery.poll() && matches(start, config.m_retryMs));
    discovery.shutdown();
    CHECK_TEST(InstallFileBackend() == 0);
    NetInterfaceDirectoryNodeData point = {};
    point.m_structSize = sizeof(point);
    point.m_version = 1;
    std::memcpy(point.m_name, "node.0123456789abcdef0123456789abcdef", 37);
    std::memcpy(point.m_address, "127.0.0.1", 10);
    point.m_capacity = point.m_boot = 1;
    /** 通过公开解析入口核对实际已发布的缓存正文，不借用生产缓存状态
    @param [in] port 本轮最后接纳快照的独立预期端口
    @return 已发布恰好一个匹配节点时返回 true
    */
    const auto savedPort = [&](uint32_t port) -> bool
    {
        NetInterfaceDirectoryHandle directory = nullptr;
        const uint64_t now = NetGatewayDirectoryStore::utc();
        if (NetInterface::directoryCreateCache(fileBackend->saved.data(), static_cast<uint32_t>(fileBackend->saved.size()),
            static_cast<uint32_t>(now), static_cast<uint32_t>(now >> 32), &directory) != NI_OK)
        {
            return false;
        }
        NetInterfaceDirectoryNodeData copied = {};
        copied.m_structSize = sizeof(copied);
        copied.m_version = 1;
        uint32_t count = 0;
        const int32_t result = NetInterface::directoryCopyNodes(directory, &copied, 1, &count);
        NetInterface::directoryDestroy(directory);
        return result == NI_OK && count == 1 && copied.m_port == port && copied.m_boot == point.m_boot;
    };
    for (unsigned mode = 0; mode < 2; ++mode)
    {
        NetGatewayDirectoryStore cache(mode == 0 ? 4093 : 0);
        execution.begin();
        CHECK_TEST(cache.poll() == NI_OK && matches(PeerOptionNow(), 60000));
        CHECK_TEST(cache.open("wait-budget.cache") == NI_OK && fileBackend && matches(PeerOptionNow(), 0));
        fileBackend->cache = true;
        // 后端已有工作而拒绝本次提交：后续 poll 有限补查，不永久等待或连续零等待。
        fileBackend->completion.id = UINT64_MAX;
        for (unsigned turn = 0; turn < 2; ++turn)
        {
            execution.begin();
            start = PeerOptionNow();
            CHECK_TEST(cache.poll() == NI_OK && matches(start, 100));
        }
        fileBackend->completion = NetFileIoResult();
        execution.begin();
        CHECK_TEST(cache.poll() == NI_OK && matches(PeerOptionNow(), 0));
        NetFileIoResult delayed = fileBackend->completion;
        CHECK_TEST(delayed.operation == NET_FILE_IO_OPEN_READ);
        fileBackend->completion = NetFileIoResult();
        for (unsigned turn = 0; turn < 2; ++turn)
        {
            execution.begin();
            start = PeerOptionNow();
            CHECK_TEST(cache.poll() == NI_OK && matches(start, 100));
        }
        fileBackend->completion = delayed;
        CHECK_TEST(cache.poll() == NI_NOT_FOUND);
        execution.begin();
        CHECK_TEST(cache.poll() == NI_NOT_FOUND && matches(PeerOptionNow(), 60000));
        CHECK_TEST(cache.queue(nullptr, 0) == NI_INVALID_ARGUMENT && matches(PeerOptionNow(), 60000));
        point.m_port = 9103;
        CHECK_TEST(cache.queue(&point, 1) == NI_OK && matches(PeerOptionNow(), 0));
        std::vector<NetInterfaceDirectoryPoint> latest(1);
        latest[0].m_name = point.m_name;
        latest[0].m_address = point.m_address;
        latest[0].m_port = 9104;
        latest[0].m_capacity = point.m_capacity;
        latest[0].m_boot = point.m_boot;
        CHECK_TEST(cache.queue(latest) == NI_OK && matches(PeerOptionNow(), 0));
        start = PeerOptionNow();
        execution.begin();
        CHECK_TEST(cache.poll() == NI_NOT_FOUND && fileBackend->completion.operation == NET_FILE_IO_OPEN_TEMPORARY &&
            matches(PeerOptionNow(), 0));
        if (mode != 0)
        {
            // 在途快照保持 9104，待写的 9105 被更新为 9106。
            point.m_port = 9105;
            CHECK_TEST(cache.queue(&point, 1) == NI_OK);
            latest[0].m_port = 9106;
            CHECK_TEST(cache.queue(latest) == NI_OK);
        }
        for (unsigned turn = 0; turn < 4 && !fileBackend->published; ++turn)
        {
            execution.begin();
            cache.poll();
            if (!fileBackend->published) { CHECK_TEST(matches(PeerOptionNow(), 0)); }
        }
        CHECK_TEST(fileBackend->published == 1 && savedPort(9104));
        if (mode == 0)
        {
            execution.begin();
            CHECK_TEST(cache.poll() == NI_OK && matches(PeerOptionNow(), 60000));
            point.m_port = 9105;
            CHECK_TEST(cache.queue(&point, 1) == NI_OK && matches(start, 4093));
            latest[0].m_port = 9106;
            CHECK_TEST(cache.queue(latest) == NI_OK && matches(start, 4093));
            execution.begin();
            CHECK_TEST(cache.poll() == NI_OK && matches(start, 4093) && !fileBackend->completion.id);
            CHECK_TEST(cache.queue(&point, 1) == NI_OK && matches(start, 4093) && fileBackend->published == 1);
        }
        else
        {
            CHECK_TEST(matches(PeerOptionNow(), 0));
            for (unsigned turn = 0; turn < 4 && fileBackend->published < 2; ++turn)
            {
                execution.begin();
                CHECK_TEST(cache.poll() == NI_OK);
                if (fileBackend->published < 2) { CHECK_TEST(matches(PeerOptionNow(), 0)); }
            }
            CHECK_TEST(fileBackend->published == 2 && savedPort(9106));
            execution.begin();
            CHECK_TEST(cache.poll() == NI_OK && matches(PeerOptionNow(), 60000));
        }
        fileBackend->m_deferDone = true;
        cache.shutdown();
        CHECK_TEST(!cache.stopped() && matches(PeerOptionNow(), 0));
        for (unsigned turn = 0; turn < 2; ++turn)
        {
            execution.begin();
            start = PeerOptionNow();
            CHECK_TEST(cache.poll() == NI_OK && !cache.stopped() && matches(start, 100));
            cache.shutdown();
            CHECK_TEST(matches(start, 100));
        }
        fileBackend->m_deferDone = false;
        if (mode != 0)
        {
            fileBackend->rejectNextRelease = true;
            execution.begin();
            start = PeerOptionNow();
            CHECK_TEST(cache.poll() == NI_INVALID_STATE && !cache.stopped() && matches(start, 100));
        }
        execution.begin();
        CHECK_TEST(cache.poll() == (mode == 0 ? NI_OK : NI_INVALID_STATE) && cache.stopped() &&
            matches(PeerOptionNow(), 60000));
        cache.shutdown();
        CHECK_TEST(matches(PeerOptionNow(), 60000));
        CHECK_TEST(cache.open("wait-budget-reopen.cache") == NI_OK && matches(PeerOptionNow(), 0));
        cache.shutdown();
        CHECK_TEST(cache.poll() == NI_OK && cache.stopped());
    }
    NetFileIo io;
    CHECK_TEST(io.init() == NI_OK);
    NetInterfaceFileInfo info = {};
    info.direction = 1;
    info.state = NI_FILE_TRANSFERRING;
    info.chunk = 1024;
    info.window = 1;
    info.total = 1024;
    info.stream = 41;
    info.expiresUtc = 3600;
    info.token[0] = info.transfer[0] = info.hash[0] = 1;
    NetFileTransfer flow;
    CHECK_TEST(flow.setProgressTimeout(1000) == NI_OK && flow.start(io, 41, info, 7, false, nullptr, nullptr, 100) == NI_OK);
    execution.begin();
    start = PeerOptionNow();
    CHECK_TEST(flow.poll(950) == NI_OK && matches(start, 150));
    NetFileIoResult closing = {};
    CHECK_TEST(flow.poll(1100) == NI_TIMEOUT && io.peek(closing) == NI_READY && closing.operation == NET_FILE_IO_CLOSE);
    CHECK_TEST(flow.onIoResult(closing) && io.consume(closing.id) == NI_OK && flow.poll(1100) == NI_TIMEOUT && flow.done());
    // 先让别的工作占用提交槽，再延迟本流已接纳的 READ。无网络事件时同样要收敛。
    NetFileTransfer sender;
    fileBackend->cache = true;
    fileBackend->saved.assign(1024, 83);
    execution.begin();
    CHECK_TEST(sender.start(io, 76, info, 7, true, nullptr, nullptr, 1100) == NI_OK && matches(PeerOptionNow(), 0));
    fileBackend->completion.id = UINT64_MAX;
    execution.begin();
    start = PeerOptionNow();
    CHECK_TEST(sender.poll(1100) == NI_NEED_MORE && matches(start, 100));
    fileBackend->completion = NetFileIoResult();
    execution.begin();
    CHECK_TEST(sender.poll(1101) == NI_OK && matches(PeerOptionNow(), 0));
    const NetFileIoResult read = fileBackend->completion;
    CHECK_TEST(read.operation == NET_FILE_IO_READ && read.file == 76 && read.size == 1024 && read.result == NI_OK);
    fileBackend->completion = NetFileIoResult();
    for (unsigned turn = 0; turn < 2; ++turn)
    {
        execution.begin();
        start = PeerOptionNow();
        CHECK_TEST(sender.poll(1102 + turn) == NI_NEED_MORE && io.peek(closing) == NI_NEED_MORE && matches(start, 100));
    }
    fileBackend->completion = read;
    CHECK_TEST(io.peek(closing) == NI_READY && sender.onIoResult(closing) && io.consume(closing.id) == NI_OK &&
        matches(PeerOptionNow(), 0));
    execution.begin();
    NetInterfaceFrame data = {};
    CHECK_TEST(sender.poll(1104) == NI_OK && sender.peekOutput(data) == NI_READY && data.m_kind == NI_DATA &&
        sender.consumeOutput() == NI_OK && matches(PeerOptionNow(), 0));
    execution.begin();
    CHECK_TEST(sender.poll(1105) == NI_OK);
    sender.cancel();
    CHECK_TEST(matches(PeerOptionNow(), 0));
    execution.begin();
    CHECK_TEST(sender.poll(1106) == NI_CANCELLED && matches(PeerOptionNow(), 0));
    execution.begin();
    start = PeerOptionNow();
    sender.cancel();
    CHECK_TEST(sender.poll(1107) == NI_NEED_MORE && matches(start, 100));
    CHECK_TEST(io.peek(closing) == NI_READY && closing.operation == NET_FILE_IO_CLOSE && closing.file == 76 &&
        sender.onIoResult(closing) && io.consume(closing.id) == NI_OK && sender.poll(1108) == NI_CANCELLED && sender.done());
    execution.begin();
    sender.cancel();
    CHECK_TEST(sender.poll(1109) == NI_CANCELLED && matches(PeerOptionNow(), 60000));
    io.stop();
    io.uninit();
    CHECK_TEST(io.released());
    return CheckFinalFileOutput(&execution, &waited);
}

/** 观察没有候选及运输连续拒绝时真实恢复期限交给宿主的等待预算
@return 选择间隔、恢复期限、初始退避及两个上限均生效，释放后停止重试时返回0
*/
static int CheckInitializedBackoff()
{
    static uint32_t s_waited = UINT32_MAX;
    NetHostExecutionFunctions host = {};
    /** 把静态观察值作为宿主实例，借用覆盖全部执行者
    @param [out] instance 接收本场景独占的毫秒观察值地址
    @return 0 表示创建成功，无动态资源
    */
    host.create = [](void** instance) -> int32_t
    {
        *instance = &s_waited;
        return 0;
    };
    /** 交换并返回先前绑定，保持嵌套执行作用域的公开契约
    @param [in] instance 本次进入的宿主实例，退出时可以为空
    @return 先前绑定的借用实例
    */
    host.exchange = [](void* instance) -> void*
    {
        static void* s_bound = nullptr;
        void* previous = s_bound;
        s_bound = instance;
        return previous;
    };
    /** 没有外部唤醒事件，使用固定观察代次
    @param [in] instance 本场景不读取的宿主实例
    @return 固定代次0
    */
    host.begin = [](void*) -> uint64_t
    {
        return 0;
    };
    /** 记录真实执行者计算的剩余预算，不等待也不推进时钟
    @param [in,out] instance 指向本轮观察值，写入请求的毫秒数
    @param [in] observed 本场景无外部事件，不使用观察代次
    @param [in] timeout 真实执行者给出的最大等待毫秒数
    @return 0 表示宿主等待完成
    */
    host.wait = [](void* instance, uint64_t, uint32_t timeout) -> int32_t
    {
        *static_cast<uint32_t*>(instance) = timeout;
        return 0;
    };
    /** 本场景同步推进，不需要外部唤醒
    @param [in] instance 本场景不读取的宿主实例
    @return 无返回值
    */
    host.wake = [](void*)
    {

    };
    /** 结束静态观察值借用，没有动态资源需要释放
    @param [in] instance 即将结束借用的宿主实例
    @return 0 表示释放成功
    */
    host.destroy = [](void*) -> int32_t
    {
        return 0;
    };
    NetHostExecutionFunctions empty = {};
    /** 所有执行者析构后恢复默认未登记状态，失败时报告诊断
    @param [in] settings 借用至返回的局部空宿主表
    @return 无返回值
    */
    const auto restore = [](NetHostExecutionFunctions* settings)
    {
        if (NetExecution::configure(*settings) != NI_OK)
        {
            std::fprintf(stderr, "Cannot release configured wait probe\n");
        }
    };
    CHECK_TEST(NetExecution::configure(host) == NI_OK);
    std::unique_ptr<NetHostExecutionFunctions, decltype(restore)> lifetime(&empty, restore);
    {
        NetExecution execution;
        NetExecutionScope scope(execution);
        CHECK_TEST(execution.status() == NI_OK && CheckClientSubmissionWait(execution, s_waited) == 0 &&
            CheckTransportWaitBudgets(execution, s_waited) == 0 &&
            CheckInitializedRecovery(false, execution, s_waited) == 0 &&
            CheckInitializedRecovery(true, execution, s_waited) == 0);
    }
    for (unsigned mode = 0; mode < 2; ++mode)
    {
        PeerServiceProbe factory;
        factory.m_refuseConnect = true;
        NetExecution execution;
        NetExecutionScope scope(execution);
        /** 设置相同100毫秒初值及120/400毫秒上限，绑定真实拒绝拨号的运输
        @param [in,out] session 初始化回调借用的真实会话
        @param [in,out] dispatch 本场景不更改的派发器
        @return 配置及绑定均成功返回 NI_OK，否则返回错误
        */
        PeerConfiguredClient client([&](NetClientSession& session, NetSenderDispatch&) -> int32_t
        {
            return session.configureRecovery(60000, 100, mode ? 400 : 120) == NI_OK ?
                session.bindGateway(&factory) : NI_INVALID_STATE;
        });
        NetPeerClientOptions options = PeerOptionConnection();
        /** 固定恢复抖动为零，其他请求仍使用测试宿主的完整随机字节
        @param [in] context 本场景不使用的随机源上下文
        @param [out] output 借用至返回的接收缓冲
        @param [in] size 四字节请求是恢复抖动，其余请求透传测试随机源
        @return 完整写入返回 NI_OK，否则返回随机源错误
        */
        options.tls.entropy = [](void*, uint8_t* output, uint32_t size) -> int32_t
        {
            if (size == sizeof(uint32_t))
            {
                std::memset(output, 0, size);
                return NI_OK;
            }
            return NetPeer::randomBytes(output, size);
        };
        NetPeer& api = NetPeer::instance();
        uint64_t failedPollStarted = PeerOptionNow();
        CHECK_TEST(client.m_created == NI_OK && execution.status() == NI_OK &&
            api.clientConnectService(client.m_handle, 7, &options) == NI_OK);
        for (unsigned attempt = 1; attempt <= 3; ++attempt)
        {
            const uint64_t guard = PeerOptionNow() + 3000;
            while (client.m_closed < attempt && PeerOptionNow() < guard)
            {
                failedPollStarted = PeerOptionNow();
                CHECK_TEST(api.clientPoll(client.m_handle) >= 0);
            }
            CHECK_TEST(client.m_closed == attempt && factory.m_created == attempt && client.m_closeReason == NI_PROTOCOL_ERROR);
            s_waited = UINT32_MAX;
            execution.begin();
            CHECK_TEST(api.clientPoll(client.m_handle) >= 0 && client.m_closed == attempt && execution.wait(120000) == NI_OK);
            const uint32_t expected = attempt == 1 ? 50 : (mode ? (attempt == 2 ? 100U : 200U) : 60U);
            // 恢复期限先于关闭回调取时；以触发失败的公开 poll 前时间作下界，容纳回调前调度停顿。
            CHECK_TEST(s_waited <= expected && s_waited + PeerOptionNow() - failedPollStarted + 2 >= expected);
        }
        CHECK_TEST(client.m_valid && client.release() == NI_OK && factory.m_created == 3 && client.m_closed == 3);
    }
    return 0;
}

/** 汇总初始化回调设置对公开客户端实际行为的影响
@return 每种独立配置的完整效果均符合预期时返回0
*/
static int CheckInitializationEffects()
{
    CHECK_TEST(CheckInitializedStorage(false) == 0 && CheckInitializedStorage(true) == 0);
    CHECK_TEST(CheckInitializedHeartbeat(false) == 0 && CheckInitializedHeartbeat(true) == 0);
    CHECK_TEST(CheckInitializedPrefetch(1, 1) == 0 && CheckInitializedPrefetch(1, 4) == 0 && CheckInitializedPrefetch(2, 4) == 0);
    CHECK_TEST(CheckInitializedService(false) == 0 && CheckInitializedService(true) == 0);
    CHECK_TEST(CheckInitializedBackoff() == 0);
    return 0;
}

PeerServiceFlow::PeerServiceFlow(const NetInterfaceTlsOptions& tls) :
m_hello(PeerOptionHello(), PeerOptionNow()),
m_dataFrames(0),
m_valid(true),
m_helloSent(false),
m_reject(false),
m_frameCalls(0),
m_firstRefusalMs(0),
m_created(0),
m_stopped(0)
{
    CEncodeDecodeTlsQueueLimits ciphertext;
    m_valid = ciphertext.set(-1, -1);
    m_server.reset(new NetInterfaceConnection(tls, NET_INTERFACE_DEFAULT_PAYLOAD_LIMIT + 32,
        NetConnectionPoolConfig(), ciphertext));
    m_valid = m_valid && m_server->status() >= 0;
}

NetGatewayTransport* PeerServiceFlow::create()
{
    ++m_created;
    return new (std::nothrow) PeerServiceFlowTransport(*this);
}

bool PeerServiceFlow::advance()
{
    for (unsigned step = 0; step < 128 && m_valid; ++step)
    {
        if (m_server->progress(PeerOptionNow()) < 0)
        {
            m_valid = false;
            break;
        }
        NetInterfaceFrame frame = {};
        const int32_t peeked = m_server->peek(frame);
        if (peeked != NI_READY)
        {
            m_valid = peeked >= 0;
            break;
        }
        if (frame.m_kind == NI_HELLO && !m_helloSent)
        {
            NetInterfaceFrame response = {};
            m_valid = m_hello.accept(frame, PeerOptionNow()) >= 0 && m_hello.peekOutput(response) == NI_READY &&
                m_server->send(response) == NI_OK && m_hello.consumeOutput() >= 0;
            m_helloSent = m_valid;
        }
        else
        {
            const uint64_t expected = m_dataFrames + 1;
            m_valid = m_helloSent && frame.m_kind == NI_DATA && frame.m_productId == 7 &&
                frame.m_messageType == 17 && frame.m_requestId == expected && frame.m_payloadSize == 16384 && frame.m_payload;
            for (uint32_t i = 0; m_valid && i < frame.m_payloadSize; ++i)
            {
                m_valid = frame.m_payload[i] == static_cast<uint8_t>(i * 17 + expected);
            }
            if (m_valid)
            {
                ++m_dataFrames;
            }
        }
        m_valid = m_valid && m_server->consume() == NI_OK;
    }
    return m_valid;
}

bool PeerServiceFlow::accept(const NetInterfaceFrame& frame)
{
    ++m_frameCalls;
    const uint64_t expected = 101 + m_events.size();
    m_valid = m_valid && !m_framesPerPoll.empty() && frame.m_kind == NI_EVENT && frame.m_productId == 7 &&
        frame.m_messageType == 17 && frame.m_requestId == expected && frame.m_payloadSize == 3 && frame.m_payload &&
        frame.m_payload[0] == expected - 100 && frame.m_payload[1] == 0 && frame.m_payload[2] == 83;
    if (m_reject)
    {
        if (!m_firstRefusalMs)
        {
            m_firstRefusalMs = PeerOptionNow();
        }
        return false;
    }
    if (m_valid)
    {
        ++m_framesPerPoll.back();
        m_events.push_back(frame.m_requestId);
    }
    return true;
}

PeerServiceFlowTransport::PeerServiceFlowTransport(PeerServiceFlow& flow) :
m_flow(flow),
m_listener(nullptr),
m_announced(false)
{

}

int32_t PeerServiceFlowTransport::connect(const char*, uint32_t, const NetInterfaceLinkOptions&,
    NetLinkTransportListener* listener, uint64_t& peer)
{
    m_listener = listener;
    peer = 1;
    return NI_OK;
}

bool PeerServiceFlowTransport::poll()
{
    if (!m_listener)
    {
        return false;
    }
    // 同一真实服务通道的每轮从运输 poll 开始；随后收帧及写记录都归入本轮。
    m_flow.m_recordsPerPoll.push_back(0);
    m_flow.m_framesPerPoll.push_back(0);
    if (!m_announced)
    {
        NetInterfaceLinkPeer identity = {};
        identity.m_role = NI_LINK_NODE;
        m_announced = true;
        m_listener->onLinkReady(1, identity);
    }
    else if (!m_openReply.empty())
    {
        const NetInterfaceFrame reply = {0, 1, NI_GATEWAY_OPEN, NI_RESPONSE,
            static_cast<uint32_t>(m_openReply.size()), reinterpret_cast<const uint8_t*>(m_openReply.data())};
        m_listener->onLinkControl(1, reply);
        m_openReply.clear();
    }
    if (!m_flow.advance())
    {
        return false;
    }
    for (unsigned step = 0; step < 128 && m_listener; ++step)
    {
        const uint8_t* bytes = nullptr;
        uint32_t size = 0;
        const int32_t pending = m_flow.m_server->peekOutput(bytes, size);
        if (pending < 0)
        {
            m_flow.m_valid = false;
            return false;
        }
        if (!size)
        {
            break;
        }
        size = (std::min)(size, 16384U);
        const NetInterfaceTunnelRecord record = {NI_TUNNEL_BYTES, 7, 1, bytes, size};
        if (!m_listener->onLinkRecord(1, record))
        {
            break;
        }
        if (m_flow.m_server->consumeOutput(size) != NI_OK)
        {
            m_flow.m_valid = false;
            return false;
        }
    }
    return m_listener != nullptr;
}

int32_t PeerServiceFlowTransport::sendControl(uint64_t peer, const NetInterfaceFrame& frame)
{
    if (peer != 1 || frame.m_productId || !frame.m_payload)
    {
        return NI_PROTOCOL_ERROR;
    }
    ProtoMessage message;
    if (!message.from(frame.m_payload, frame.m_payloadSize))
    {
        return NI_PROTOCOL_ERROR;
    }
    const uint8_t one[8] = {1};
    const Variant* channel = message.findValue(NI_GATEWAY_CHANNEL);
    if (!channel || channel->type() != Variant::UINT64 || channel->size() != 8 ||
        std::memcmp(channel->data(), one, sizeof(one)))
    {
        return NI_PROTOCOL_ERROR;
    }
    if (frame.m_kind == NI_EVENT && frame.m_requestId == 0 && frame.m_messageType == NI_GATEWAY_CREDIT)
    {
        const Variant* credit = message.findValue(NI_GATEWAY_CREDIT_BYTES);
        return credit && credit->type() == Variant::INT32 && credit->size() == 4 ? NI_OK : NI_PROTOCOL_ERROR;
    }
    const uint8_t target[8] = {7};
    const uint8_t window[4] = {0, 0, 16, 0};
    const Variant* product = message.findValue(NI_GATEWAY_TARGET_PRODUCT);
    const Variant* offered = message.findValue(NI_GATEWAY_WINDOW);
    if (frame.m_kind != NI_REQUEST || frame.m_requestId != 1 || frame.m_messageType != NI_GATEWAY_OPEN ||
        !product || product->type() != Variant::UINT64 || product->size() != 8 ||
        std::memcmp(product->data(), target, sizeof(target)) || !offered || offered->type() != Variant::INT32 ||
        offered->size() != 4 || std::memcmp(offered->data(), window, sizeof(window)))
    {
        return NI_PROTOCOL_ERROR;
    }
    ProtoMessage response;
    return response.setBytes(NI_GATEWAY_CHANNEL, one, sizeof(one), Variant::UINT64) &&
        response.setBytes(NI_GATEWAY_WINDOW, window, sizeof(window), Variant::INT32) &&
        SerializeTestReply(response, m_openReply) ? NI_OK : NI_NO_MEMORY;
}

int32_t PeerServiceFlowTransport::sendRecord(uint64_t peer, const NetInterfaceTunnelRecord& record)
{
    if (peer != 1 || record.m_kind != NI_TUNNEL_BYTES || record.m_product != 7 || record.m_channel != 1 ||
        !record.m_data || !record.m_size || record.m_size > 16384 || m_flow.m_recordsPerPoll.empty())
    {
        return NI_PROTOCOL_ERROR;
    }
    uint32_t consumed = 0;
    const int32_t result = m_flow.m_server->feed(record.m_data, record.m_size, consumed);
    if (result < 0 || consumed != record.m_size)
    {
        return NI_PROTOCOL_ERROR;
    }
    ++m_flow.m_recordsPerPoll.back();
    return NI_OK;
}

void PeerServiceFlowTransport::shutdown()
{
    if (m_listener)
    {
        ++m_flow.m_stopped;
        m_listener = nullptr;
    }
}

int32_t PeerServiceFlowTransport::outputState(uint64_t peer) const
{
    return peer == 1 && m_listener ? NI_OK : NI_INVALID_STATE;
}

/** 完成真实服务 TLS 和业务 HELLO，保留正式就绪回调
@param [in,out] client 经 m_configure 绑定本组工厂的客户端
@param [in,out] flow 独立的真实服务对端
@return 服务 TLS 和业务握手都完成时返回0
*/
static int ReadyServiceOptions(PeerConfiguredClient& client, PeerServiceFlow& flow)
{
    NetPeer& api = NetPeer::instance();
    const NetPeerClientOptions options = PeerOptionConnection();
    /** 将真实应用帧交给比客户端长寿的观察者
    @param [in] frame 仅借用至回调返回的业务帧
    @return true 接纳该帧，false 保持背压；更新 flow 的正文和次数观察
    */
    client.m_onFrame = [&flow](const NetInterfaceFrame& frame) -> bool
    {
        return flow.accept(frame);
    };
    CHECK_TEST(flow.m_valid && client.m_created == NI_OK && api.clientConnectService(client.m_handle, 7, &options) == NI_OK);
    const uint64_t guard = PeerOptionNow() + 5000;
    while (!client.m_ready && !client.m_closed && PeerOptionNow() < guard)
    {
        CHECK_TEST(api.clientPoll(client.m_handle) >= 0 && flow.m_valid);
    }
    CHECK_TEST(client.m_ready == 1 && !client.m_closed && client.m_valid && flow.m_created == 1 &&
        flow.m_helloSent && flow.m_server->status() == NET_INTERFACE_TLS_READY);
    CHECK_TEST(api.clientPoll(client.m_handle) >= 0 && api.clientPoll(client.m_handle) >= 0);
    return 0;
}

/** 在正式服务通道里核对接收帧和输出密文记录的单轮量子
@param [in] tls 隔离 service.7 服务端身份
@param [in] budget 接收和发送各自设置的预算，使用1或3
@return 两方向峰值恰为预算、所有正文顺序正确且只关闭一次时返回0
*/
static int CheckServiceBudgets(const NetInterfaceTlsOptions& tls, uint32_t budget)
{
    PeerServiceFlow flow(tls);
    /** 在初始化阶段配置实际服务通道，回调返回后不保留配置引用
    @param [in,out] session 待发布客户端的会话
    @param [in,out] dispatch 本场景不更改的派发器
    @return 所有配置及绑定成功返回 NI_OK，否则返回错误
    */
    PeerConfiguredClient client([&](NetClientSession& session, NetSenderDispatch&) -> int32_t
    {
        CEncodeDecodeTlsQueueLimits limits;
        return limits.set(-1, -1) && session.configureServiceTlsQueueLimits(limits) == NI_OK &&
            session.configureServiceWindow(1048576) == NI_OK && session.configureServicePollBudgets(budget, budget) == NI_OK &&
            session.configureRecovery(60000, 60000, 60000) == NI_OK ? session.bindGateway(&flow) : NI_INVALID_STATE;
    });
    CHECK_TEST(ReadyServiceOptions(client, flow) == 0);
    flow.m_recordsPerPoll.clear();
    flow.m_framesPerPoll.clear();
    for (uint32_t i = 0; i < 7; ++i)
    {
        const uint8_t body[] = {static_cast<uint8_t>(i + 1), 0, 83};
        const NetInterfaceFrame frame = {7, 101 + i, 17, NI_EVENT, sizeof(body), body};
        CHECK_TEST(flow.m_server->send(frame) == NI_OK);
    }
    NetPeer& api = NetPeer::instance();
    for (unsigned step = 0; step < 256 && flow.m_events.size() != 7 && !client.m_closed; ++step)
    {
        CHECK_TEST(api.clientPoll(client.m_handle) >= 0 && flow.m_valid);
    }
    CHECK_TEST(flow.m_events.size() == 7 && flow.m_frameCalls == 7 && !flow.m_framesPerPoll.empty() &&
        *std::max_element(flow.m_framesPerPoll.begin(), flow.m_framesPerPoll.end()) == budget);
    flow.m_recordsPerPoll.clear();
    flow.m_framesPerPoll.clear();
    std::vector<uint8_t> body(16384);
    for (uint32_t id = 1; id <= 4; ++id)
    {
        for (size_t i = 0; i < body.size(); ++i)
        {
            body[i] = static_cast<uint8_t>(i * 17 + id);
        }
        const NetInterfaceFrame frame = {7, id, 17, NI_DATA, static_cast<uint32_t>(body.size()), body.data()};
        CHECK_TEST(api.clientPostData(client.m_handle, &frame) == NI_OK);
    }
    std::fill(body.begin(), body.end(), 0);
    for (unsigned step = 0; step < 512 && flow.m_dataFrames != 4 && !client.m_closed; ++step)
    {
        CHECK_TEST(api.clientPoll(client.m_handle) >= 0 && flow.m_valid);
    }
    CHECK_TEST(flow.m_dataFrames == 4 && !flow.m_recordsPerPoll.empty() &&
        *std::max_element(flow.m_recordsPerPoll.begin(), flow.m_recordsPerPoll.end()) == budget);
    CHECK_TEST(client.m_valid && !client.m_closed && client.release() == NI_OK && client.m_closed == 1 && flow.m_stopped == 1);
    return 0;
}

/** 从真实业务帧首次被应用拒收起核对短/长期限
@param [in] tls 隔离 service.7 服务端身份
@param [in] shortDeadline true设置80毫秒，false设置60000毫秒
@return 短期限只关闭、长期限恢复后只交付一次原帧时返回0
*/
static int CheckServiceBackpressure(const NetInterfaceTlsOptions& tls, bool shortDeadline)
{
    PeerServiceFlow flow(tls);
    /** 在初始化阶段为实际服务通道设置应用拒收期限
    @param [in,out] session 待发布客户端的会话
    @param [in,out] dispatch 本场景不更改的派发器
    @return 所有配置及绑定成功返回 NI_OK，否则返回错误
    */
    PeerConfiguredClient client([&](NetClientSession& session, NetSenderDispatch&) -> int32_t
    {
        CEncodeDecodeTlsQueueLimits limits;
        return limits.set(-1, -1) && session.configureServiceTlsQueueLimits(limits) == NI_OK &&
            session.configureServiceWindow(1048576) == NI_OK &&
            session.configureServiceTimeouts(60000, shortDeadline ? 80 : 60000) == NI_OK &&
            session.configureRecovery(60000, 60000, 60000) == NI_OK ? session.bindGateway(&flow) : NI_INVALID_STATE;
    });
    CHECK_TEST(ReadyServiceOptions(client, flow) == 0);
    flow.m_reject = true;
    const uint8_t body[] = {1, 0, 83};
    const NetInterfaceFrame frame = {7, 101, 17, NI_EVENT, sizeof(body), body};
    CHECK_TEST(flow.m_server->send(frame) == NI_OK);
    NetPeer& api = NetPeer::instance();
    uint64_t firstRefusalPoll = 0;
    for (unsigned step = 0; step < 256 && !flow.m_frameCalls && !client.m_closed; ++step)
    {
        // 内部按本轮取时启动期限；用公开 poll 前的时间作下界，不把回调调度耗时误算成早到。
        firstRefusalPoll = PeerOptionNow();
        CHECK_TEST(api.clientPoll(client.m_handle) >= 0 && flow.m_valid);
    }
    CHECK_TEST(flow.m_frameCalls && flow.m_firstRefusalMs && flow.m_events.empty() && !client.m_closed);
    const uint64_t until = flow.m_firstRefusalMs + (shortDeadline ? 3000 : 120);
    while (!client.m_closed && PeerOptionNow() < until)
    {
        CHECK_TEST(api.clientPoll(client.m_handle) >= 0 && flow.m_valid);
    }
    CHECK_TEST(flow.m_events.empty());
    if (shortDeadline)
    {
        CHECK_TEST(client.m_closed == 1 && client.m_closeReason == NI_TIMEOUT &&
            client.m_closedAt + 2 >= firstRefusalPoll + 80);
    }
    else
    {
        CHECK_TEST(!client.m_closed && PeerOptionNow() >= flow.m_firstRefusalMs + 120);
        flow.m_reject = false;
        for (unsigned step = 0; step < 16 && flow.m_events.empty(); ++step)
        {
            CHECK_TEST(api.clientPoll(client.m_handle) >= 0 && flow.m_valid);
        }
        CHECK_TEST(flow.m_events.size() == 1 && flow.m_events[0] == 101 && !client.m_closed);
        CHECK_TEST(api.clientPoll(client.m_handle) >= 0 && flow.m_events.size() == 1);
    }
    CHECK_TEST(client.m_valid && flow.m_valid && client.release() == NI_OK && client.m_closed == 1 && flow.m_stopped == 1);
    return 0;
}

/** 有界读取隔离叶身份文件并为 PEM 解析附加终止符
@param [in] path 本次测试素材路径，不修改文件
@param [out] bytes 调用方容量至少16385字节，借用至返回
@param [out] size 包含新增终止符的实际长度
@return 文件非空、至多16384字节且读取成功时返回0
*/
static int ReadServicePem(const char* path, uint8_t* bytes, uint32_t& size)
{
    std::unique_ptr<FILE, int (*)(FILE*)> file(std::fopen(path, "rb"), std::fclose);
    CHECK_TEST(file.get());
    const size_t count = std::fread(bytes, 1, 16385, file.get());
    CHECK_TEST(!std::ferror(file.get()) && count != 0 && count < 16385);
    bytes[count] = 0;
    size = static_cast<uint32_t>(count + 1);
    return 0;
}

/** 用隔离 service.7 叶身份执行完整服务配置效果专项
@param [in] certificate 公开测试叶证书路径
@param [in] privateKey 匹配的隔离叶私钥路径，不得使用正式服务身份
@return 四组真实服务 TLS、预算与背压场景均符合预期时返回0
*/
static int ServiceOptions(const char* certificate, const char* privateKey)
{
    CHECK_TEST(InstallWire() == NI_OK);
    /** 包括失败样本在内，释放前擦除独占的两份读取缓冲
    @param [in,out] storage 容量为32770字节的独占数组；释放后不得继续使用
    @return 无返回值
    */
    const auto release = [](uint8_t* storage)
    {
        NetInterface::wipe(storage, 32770);
        delete[] storage;
    };
    std::unique_ptr<uint8_t[], decltype(release)> storage(new uint8_t[32770](), release);
    NetInterfaceTlsOptions tls = {};
    tls.server = 1;
    tls.handshakeTimeoutMs = 5000;
    tls.certificate = storage.get();
    tls.privateKey = storage.get() + 16385;
    /** 使用已安装的确定性测试宿主，仅驱动隔离 TLS
    @param [in] context 本场景不使用的宿主上下文
    @param [out] output 接收随机字节的缓冲，借用至返回
    @param [in] size 需要写入的字节数
    @return 完整写入返回 NI_OK，否则返回宿主错误
    */
    tls.entropy = [](void*, uint8_t* output, uint32_t size) -> int32_t
    {
        return NetPeer::randomBytes(output, size);
    };
    CHECK_TEST(ReadServicePem(certificate, storage.get(), tls.certificateSize) == 0 &&
        ReadServicePem(privateKey, storage.get() + 16385, tls.privateKeySize) == 0);
    CHECK_TEST(CheckServiceBudgets(tls, 1) == 0 && CheckServiceBudgets(tls, 3) == 0);
    CHECK_TEST(CheckServiceBackpressure(tls, true) == 0 && CheckServiceBackpressure(tls, false) == 0);
    std::puts("NetPeerServiceOptions passed");
    return 0;
}

/** 配置后完成真实业务握手，区分提交入口和会话请求池的槽位
@param [in] events 有效的客户端回调表
@param [in] options 可复制的客户端连接参数
@param [in] commands 每轮最大命令数
@param [in] bytes 每轮最大正文字节数
@param [in] deliveryTimeoutMs 非零时验证此同步交付期限，单位毫秒
@return 通道参数、分配和背压均与预期一致时返回0
*/
static int CheckConfiguredRequests(const NetPeerClientCallbacks& events, NetPeerClientOptions options,
    uint32_t commands, uint32_t bytes, uint32_t deliveryTimeoutMs = 0)
{
    NetInterfaceHandshakeOptions offer = {};
    offer.m_server = 1;
    // 协商 RPC、DATA 和公开命令压缩，三类行为均经同一真实会话验证。
    offer.m_capabilities = 1 | 2 | 8;
    offer.m_maxRequests = 4;
    offer.m_maxChunk = 4096;
    offer.m_timeoutMs = 5000;
    offer.m_nonce[0] = 19;
    NetInterfaceHandshake remote(offer, 0);
    PeerConfigChannel channel(remote);
    NetPeerConfig config;
    PeerDispatchProbe probe = {&channel, commands, bytes};
    config.m_context = &probe;
    config.m_configure = [](void* context, NetClientSession& session, NetSenderDispatch& dispatch) -> int32_t {
        const PeerDispatchProbe& settings = *static_cast<PeerDispatchProbe*>(context);
        const int32_t result = dispatch.configurePollBudgets(settings.m_commands, settings.m_bytes);
        return result == NI_OK ? session.bindChannel(settings.m_channel) : result;
    };
    size_t sessionGrowth = 0, frontGrowth = 0, queueGrowth = 0;
    NetPeer& api = NetPeer::instance();
    NetPeerClientHandle handle = nullptr;
    CHECK_TEST(api.clientCreate(&events, &handle, config) == NI_OK);
    const auto release = [&](NetPeerClientHandle remaining) {
        api.clientClose(remaining, 0);
        for (unsigned i = 0; i < 16; ++i)
        {
            api.clientPoll(remaining);
            if (api.clientDestroy(remaining) == NI_OK) { break; }
        }
    };
    std::unique_ptr<NetPeerClientOpaque, decltype(release)> owner(handle, release);
    NetConnectionPoolConfig connection;
    connection.m_decoderRetentionBytes = -1;
    connection.m_compressionMinPayload = 23;
    connection.m_controlBurst = 3;
    connection.m_controlPayloadLimit = -1;
    connection.m_progressFrameBudget = 13;
    connection.m_progressStepBudget = 19;
    connection.m_compressionLevel = 7;
    CHECK_TEST(api.clientConfigureConnectionPools(handle, connection) == NI_OK);
    connection = NetConnectionPoolConfig();
    LockFreeHashMapPoolConfig pending;
    ObservePool(pending, sessionGrowth);
    NetSenderSubmissionPools submission;
    ObservePool(submission.m_fronts, frontGrowth);
    submission.m_fronts.setMaximumCapacity(1);
    submission.m_queue.initialCapacity = 2;
    submission.m_queue.maximumCapacity = -1;
    NetSenderSubmissionOptions policy;
    policy.m_growPolicy = [&](const LockFreeQueueGrowContext&) -> size_t { ++queueGrowth; return 4; };
    if (deliveryTimeoutMs) { policy.m_deliveryTimeoutMs = deliveryTimeoutMs; }
    CHECK_TEST(api.clientConfigureRequestPool(handle, pending) == NI_OK &&
        api.clientConfigureSubmissionPools(handle, submission, policy) == NI_OK);
    policy.m_growPolicy = nullptr;
    policy.m_deliveryTimeoutMs = 1;
    uint32_t types[] = {17, 257};
    CHECK_TEST(api.clientConfigureCompression(handle, types, 2) == NI_OK);
    types[0] = 99;
    options.capabilities = offer.m_capabilities;
    CHECK_TEST(api.clientConnect(handle, &options) == NI_OK);
    int32_t ready = 0;
    uint64_t generation = 0;
    for (unsigned i = 0; !ready && i < 16; ++i)
    {
        CHECK_TEST(api.clientPoll(handle) >= 0 && api.clientState(handle, &generation, &ready) == NI_OK);
    }
    CHECK_TEST(ready && generation && channel.m_types.size() == 2 && channel.m_types[0] == 17 && channel.m_types[1] == 257);
    CHECK_TEST(channel.m_received.m_decoderRetentionBytes == -1 && channel.m_received.m_compressionMinPayload == 23 &&
        channel.m_received.m_controlBurst == 3 && channel.m_received.m_controlPayloadLimit == -1 &&
        channel.m_received.m_progressFrameBudget == 13 && channel.m_received.m_progressStepBudget == 19 &&
        channel.m_received.m_compressionLevel == 7);
    uint64_t first = 0, second = 91;
    CHECK_TEST(api.clientBegin(handle, 7, 17, 5000, &first) == NI_OK && first && frontGrowth == 1 && sessionGrowth == 0);
    CHECK_TEST(api.clientBegin(handle, 7, 17, 5000, &second) == NI_LIMIT_EXCEEDED && second == 0);
    const uint8_t body = 73;
    CHECK_TEST(api.clientPost(handle, first, &body, 1) == NI_OK);
    for (unsigned i = 0; !channel.m_requests && i < 16; ++i) { CHECK_TEST(api.clientPoll(handle) >= 0); }
    CHECK_TEST(channel.m_requests == 1 && sessionGrowth == 1 && frontGrowth == 1);
    const size_t initialGrowth = queueGrowth;
    for (uint32_t i = 1; i <= 32; ++i)
    {
        const uint8_t byte = static_cast<uint8_t>(i);
        const NetInterfaceFrame frame = {7, i, 0, NI_DATA, 1, &byte};
        CHECK_TEST(api.clientPostData(handle, &frame) == NI_OK);
    }
    CHECK_TEST(queueGrowth > initialGrowth && api.clientConfigureSubmissionPools(handle, submission, policy) == NI_INVALID_STATE);
    const uint32_t limit = (std::min)(commands, bytes);
    uint32_t peak = 0;
    for (unsigned i = 0; channel.m_dataFrames < 32 && i < 64; ++i)
    {
        const uint32_t before = channel.m_dataFrames;
        CHECK_TEST(api.clientPoll(handle) >= 0 && channel.m_dataFrames <= before + limit);
        peak = (std::max)(peak, channel.m_dataFrames - before);
    }
    CHECK_TEST(channel.m_dataFrames == 32 && peak == limit);
    if (deliveryTimeoutMs)
    {
        channel.m_blockData = true;
        const uint32_t before = channel.m_dataOffers;
        uint8_t byte = 33;
        const NetInterfaceFrame frame = {7, 33, 0, NI_DATA, 1, &byte};
        uint64_t ticket = 0;
        const auto started = PeerOptionNow();
        CHECK_TEST(api.clientSendDataBegin(handle, &frame, &ticket) == NI_OK && ticket);
        byte = 0;
        int32_t result = api.clientSendResult(handle, ticket);
        CHECK_TEST(result == NI_READY);
        const auto observeUntil = started + (200);
        while (PeerOptionNow() < observeUntil)
        {
            CHECK_TEST(api.clientPoll(handle) >= 0 && channel.m_dataFrames == 32);
            if (result == NI_READY)
            {
                result = api.clientSendResult(handle, ticket);
                if (result != NI_READY)
                {
                    CHECK_TEST(deliveryTimeoutMs == 100 && result == NI_TIMEOUT &&
                        PeerOptionNow() >= started + (98));
                }
            }
        }
        CHECK_TEST(channel.m_dataOffers > before && result == (deliveryTimeoutMs == 100 ? NI_TIMEOUT : NI_READY));
        channel.m_blockData = false;
        for (unsigned i = 0; i < 16; ++i)
        {
            CHECK_TEST(api.clientPoll(handle) >= 0);
            if (result == NI_READY) { result = api.clientSendResult(handle, ticket); }
        }
        CHECK_TEST(result == (deliveryTimeoutMs == 100 ? NI_TIMEOUT : NI_OK));
        CHECK_TEST(channel.m_dataFrames == (deliveryTimeoutMs == 100 ? 32U : 33U));
        CHECK_TEST(api.clientSendResult(handle, ticket) == NI_NOT_FOUND);
        CHECK_TEST(api.clientState(handle, &generation, &ready) == NI_OK && ready && generation);
    }
    CHECK_TEST(api.clientClose(handle, NI_CANCELLED) == NI_OK);
    int32_t destroyed = NI_INVALID_STATE;
    for (unsigned i = 0; i < 16 && destroyed == NI_INVALID_STATE; ++i)
    {
        CHECK_TEST(api.clientPoll(handle) >= 0);
        destroyed = api.clientDestroy(handle);
    }
    CHECK_TEST(destroyed == NI_OK);
    owner.release();
    return 0;
}

/** 从公开客户端句柄拨号，核对三条路径复制的池及关闭收敛。
@return 全部通过为0
*/
static int ClientPaths()
{
    CHECK_TEST(InstallWire() == NI_OK);
    NetPeer& api = NetPeer::instance();
    NetPeerConfig cfg;
    size_t direct = 0, service = 0, discovery = 0;
    ObservePool(cfg.m_direct.m_peers, direct);
    ObservePool(cfg.m_service.m_peers, service);
    ObservePool(cfg.m_discovery.m_transport.m_peers, discovery);
    NetPeerClientCallbacks events = {};
    events.structSize = sizeof(events);
    events.version = 1;
    events.ready = [](void*, uint64_t, uint32_t, uint32_t) {};
    events.result = [](void*, uint64_t, uint64_t, uint32_t, uint32_t, uint32_t, const uint8_t*, uint32_t) {};
    events.frame = [](void*, uint64_t, const NetInterfaceFrame*) -> int32_t { return 1; };
    events.closed = [](void*, uint64_t, int32_t, int32_t) {};
    static const uint8_t ca[] = TEST_CA_CRT_RSA_SHA256_PEM;
    NetPeerClientOptions opts = {};
    opts.structSize = sizeof(opts);
    opts.version = 1;
    opts.address = "127.0.0.1";
    opts.port = 9123;
    opts.platform = 1;
    opts.capabilities = 7;
    opts.maxRequests = 4;
    opts.maxChunk = 4096;
    opts.handshakeTimeoutMs = 1000;
    opts.tls.authority = ca;
    opts.tls.authoritySize = sizeof(ca);
    opts.tls.hostname = "localhost";
    opts.device[0] = 1;
    for (int route = 0; route < 2; ++route)
    {
        NetPeerClientHandle handle = nullptr;
        CHECK_TEST(api.clientCreate(&events, &handle, cfg) == NI_OK && handle);
        // 失败也须收敛借用的配置观察者，不能把客户端留给下一项用例。
        const auto release = [&api](NetPeerClientHandle remaining) {
            api.clientClose(remaining, NI_CANCELLED);
            for (unsigned i = 0; i < 16; ++i)
            {
                api.clientPoll(remaining);
                if (api.clientDestroy(remaining) == NI_OK) { break; }
            }
        };
        std::unique_ptr<NetPeerClientOpaque, decltype(release)> owner(handle, release);
        CHECK_TEST(api.clientConfigureGateways(handle, "127.0.0.1:9102", nullptr) == NI_OK);
        CHECK_TEST((route ? api.clientConnectService(handle, 1, &opts) : api.clientConnect(handle, &opts)) == NI_OK);
        for (int i = 0; i < 4; ++i) { CHECK_TEST(api.clientPoll(handle) >= 0); }
        CHECK_TEST(direct == 1 && (route ? service > 0 && discovery > 0 : !service && !discovery));
        CHECK_TEST(api.clientClose(handle, 0) == NI_OK);
        int32_t destroyed = NI_INVALID_STATE;
        for (int i = 0; i < 16 && destroyed == NI_INVALID_STATE; ++i)
        {
            api.clientPoll(handle);
            destroyed = api.clientDestroy(handle);
        }
        CHECK_TEST(destroyed == NI_OK);
        owner.release();
    }
    CHECK_TEST(CheckConfiguredRequests(events, opts, 1, 1, 100) == 0);
    CHECK_TEST(CheckConfiguredRequests(events, opts, 8, 3, 1000) == 0);
    CHECK_TEST(CheckConfiguredRequests(events, opts, 2, 32) == 0);
    return CheckInitializationEffects();
}

/** 通过真实 TLS 服务端检查帧顺序、预算及 EOF 后的接收背压
@param [in] drainMs 当前实例从远端 EOF 开始的排空期限，单位毫秒
@param [in] largeWrite 是否改为验证大帧积压后的密文切分
@param [in] singleEvent 是否每轮仅派发一个原始事件，核对可写通知、输入与 EOF 顺序
@return 全部通过为0
*/
static int CheckTlsFrames(uint32_t drainMs, bool largeWrite = false, bool singleEvent = false)
{
    CHECK_TEST(InstallWire() == NI_OK);
    static const uint8_t ca[] = TEST_CA_CRT_RSA_SHA256_PEM;
    static const uint8_t cert[] = TEST_SRV_CRT_RSA_SHA256_PEM;
    static const uint8_t key[] = TEST_SRV_KEY_RSA_PEM;
    NetInterfaceTlsOptions clientTls = {};
    clientTls.authority = ca; clientTls.authoritySize = sizeof(ca); clientTls.hostname = "localhost";
    NetInterfaceTlsOptions serverTls = {};
    serverTls.server = 1;
    serverTls.certificate = cert; serverTls.certificateSize = sizeof(cert);
    serverTls.privateKey = key; serverTls.privateKeySize = sizeof(key);
    serverTls.entropy = [](void*, uint8_t* out, uint32_t size) -> int32_t { return NetPeer::randomBytes(out, size); };
    NetInterfaceConnection server(serverTls);
    NetDirectTransportConfig config;
    config.m_payloadLimit = largeWrite ? 131072 : 64;
    config.m_frameBudget = largeWrite ? 8 : 1;
    if (singleEvent) { config.m_host.m_eventBudget = 1; }
    if (largeWrite) { config.m_outputBudget = 49152; }
    config.m_drainTimeoutMs = drainMs;
    CHECK_TEST(config.m_ciphertext.set(-1, -1));
    PeerTestListener listener;
    NetTransport client(config);
    NetConnectionPoolConfig wirePool;
    wirePool.m_dataRecords.setInitialCapacity(1);
    wirePool.m_dataRecords.setMaximumCapacity(1);
    CHECK_TEST(client.configureConnectionPools(wirePool) == NI_OK);
    wirePool.m_progressFrameBudget = 0;
    CHECK_TEST(client.configureConnectionPools(wirePool) == NI_INVALID_ARGUMENT);
    uint64_t id = 0;
    CHECK_TEST(client.connect("127.0.0.1", 9103, clientTls, &listener, id) == NI_OK && activeWire);
    activeWire->capture = true;
    activeWire->connected(activeWire->observer, id);
    // 每次至多交给宿主1024字节，在下一轮poll中驱动真实TLS输入。
    const auto advance = [&]() -> bool {
        if (!client.poll() || !activeWire->live || server.progress(1000) < 0) { return false; }
        std::vector<uint8_t>& input = activeWire->output;
        if (!input.empty())
        {
            uint32_t used = 0;
            if (server.feed(input.data(), static_cast<uint32_t>(input.size()), used) < 0 || used > input.size()) { return false; }
            input.erase(input.begin(), input.begin() + used);
        }
        const uint8_t* data = nullptr;
        uint32_t size = 0;
        if (server.peekOutput(data, size) < 0) { return false; }
        if (size && !activeWire->paused)
        {
            if (size > 1024) { size = 1024; }
            if (singleEvent) { activeWire->writable(activeWire->observer, id); }
            activeWire->data(activeWire->observer, id, data, size);
            if (server.consumeOutput(size) != NI_OK) { return false; }
        }
        return true;
    };
    for (int i = 0; i < 2048 && (!listener.readyCount || server.status() != NET_INTERFACE_TLS_READY); ++i) { CHECK_TEST(advance()); }
    CHECK_TEST(listener.readyCount == 1 && server.status() == NET_INTERFACE_TLS_READY);
    if (largeWrite)
    {
        std::vector<uint8_t> payload(98304);
        for (size_t i = 0; i < payload.size(); ++i) { payload[i] = static_cast<uint8_t>(i * 17 + i / 251); }
        NetInterfaceFrame frame = {};
        frame.m_kind = NI_DATA;
        frame.m_requestId = 123;
        frame.m_payload = payload.data();
        frame.m_payloadSize = static_cast<uint32_t>(payload.size());
        activeWire->m_blockWrites = true;
        CHECK_TEST(client.send(id, frame) == NI_OK);
        for (int i = 0; i < 12; ++i) { CHECK_TEST(advance()); }
        CHECK_TEST(activeWire->m_writeRefusals && !listener.closeCount);
        activeWire->m_blockWrites = false;
        activeWire->writable(activeWire->observer, id);
        NetInterfaceFrame received = {};
        for (int i = 0; i < 2048 && server.peek(received) != NI_READY; ++i)
        {
            const uint32_t before = activeWire->writes;
            CHECK_TEST(advance() && activeWire->writes - before <= config.m_outputBudget);
        }
        CHECK_TEST(server.peek(received) == NI_READY && received.m_requestId == 123 && received.m_kind == NI_DATA &&
            received.m_payloadSize == payload.size() && std::memcmp(received.m_payload, payload.data(), payload.size()) == 0);
        CHECK_TEST(server.consume() == NI_OK && server.peek(received) != NI_READY && !listener.closeCount &&
            activeWire->m_largestWrite <= 32768);
        client.shutdown();
        return 0;
    }
    uint8_t body[] = {6, 5, 4};
    NetInterfaceFrame frame = {};
    frame.m_kind = NI_REQUEST; frame.m_requestId = 101; frame.m_messageType = 7;
    frame.m_payload = body; frame.m_payloadSize = sizeof(body);
    CHECK_TEST(client.send(id, frame) == NI_OK);
    CHECK_TEST(client.send(id, frame) == NI_LIMIT_EXCEEDED);
    body[0] = 0;
    NetInterfaceFrame received = {};
    for (int i = 0; i < 2048 && server.peek(received) != NI_READY; ++i) { CHECK_TEST(advance()); }
    CHECK_TEST(server.peek(received) == NI_READY && received.m_requestId == 101 && received.m_payloadSize == 3 &&
        received.m_payload[0] == 6 && received.m_payload[1] == 5 && received.m_payload[2] == 4 && server.consume() == NI_OK);
    for (uint8_t i = 1; i <= 3; ++i)
    {
        frame.m_requestId = 200 + i;
        body[0] = i;
        CHECK_TEST(server.send(frame) == NI_OK);
    }
    for (int i = 0; i < 2048 && listener.frames.size() < 3; ++i)
    {
        const size_t before = listener.frames.size();
        CHECK_TEST(advance() && listener.frames.size() - before <= 1);
    }
    CHECK_TEST(listener.frames.size() == 3 && listener.frames[0] == 201 && listener.frames[1] == 202 && listener.frames[2] == 203);
    CHECK_TEST(listener.bodies.size() == 3 && listener.bodies[0] == 1 && listener.bodies[1] == 2 && listener.bodies[2] == 3);
    CHECK_TEST(client.configureCompact(0, true) == NI_INVALID_STATE && client.configureCompact(id, true) == NI_OK);
    server.configureCompact(true);
    frame.m_kind = NI_DATA;
    for (uint8_t i = 4; i <= 5; ++i)
    {
        frame.m_requestId = 200 + i;
        body[0] = i;
        CHECK_TEST(server.send(frame) == NI_OK);
    }
    for (int i = 0; i < 2048 && listener.frames.size() < 5; ++i) { CHECK_TEST(advance()); }
    CHECK_TEST(listener.frames.size() == 5 && listener.frames[3] == 204 && listener.frames[4] == 205 &&
        listener.bodies.size() == 5 && listener.bodies[3] == 4 && listener.bodies[4] == 5);
    server.configureCompact(false);
    CHECK_TEST(client.configureCompact(id, false) == NI_OK);
    frame.m_requestId = 301;
    body[0] = 11;
    CHECK_TEST(client.send(id, frame) == NI_OK);
    for (int i = 0; i < 2048 && server.peek(received) != NI_READY; ++i) { CHECK_TEST(advance()); }
    CHECK_TEST(server.peek(received) == NI_READY && received.m_kind == NI_DATA && received.m_requestId == 301 &&
        received.m_payloadSize == 3 && received.m_payload[0] == 11 && server.consume() == NI_OK);
    uint8_t boundary[65] = {};
    for (uint32_t i = 0; i < sizeof(boundary); ++i) { boundary[i] = static_cast<uint8_t>(i); }
    frame.m_requestId = 302;
    frame.m_payload = boundary;
    frame.m_payloadSize = sizeof(boundary);
    CHECK_TEST(client.send(id, frame) == NI_LIMIT_EXCEEDED);
    frame.m_payloadSize = config.m_payloadLimit;
    CHECK_TEST(client.send(id, frame) == NI_OK);
    for (int i = 0; i < 2048 && server.peek(received) != NI_READY; ++i) { CHECK_TEST(advance()); }
    CHECK_TEST(server.peek(received) == NI_READY && received.m_requestId == 302 && received.m_payloadSize == 64 &&
        std::memcmp(received.m_payload, boundary, 64) == 0 && server.consume() == NI_OK);

    if (singleEvent)
    {
        // 同一输入含两条完整 TLS 帧。宿主随后报告 EOF，本地已有待发帧。
        frame.m_payload = body;
        frame.m_payloadSize = sizeof(body);
        for (uint64_t request : {401U, 402U})
        {
            frame.m_requestId = request;
            body[0] = static_cast<uint8_t>(request - 400);
            CHECK_TEST(server.send(frame) == NI_OK);
        }
        std::vector<uint8_t> tail;
        for (unsigned step = 0; step < 16 && server.outputState() == NI_READY; ++step)
        {
            CHECK_TEST(server.progress(1000) >= 0);
            const uint8_t* bytes = nullptr;
            uint32_t count = 0;
            CHECK_TEST(server.peekOutput(bytes, count) == NI_OK);
            if (count)
            {
                tail.insert(tail.end(), bytes, bytes + count);
                CHECK_TEST(server.consumeOutput(count) == NI_OK);
            }
        }
        CHECK_TEST(!tail.empty() && tail.size() <= 16384 && server.outputState() == NI_OK && !activeWire->paused);
        CHECK_TEST(client.send(id, frame) == NI_OK);
        const uint32_t written = activeWire->writes;
        activeWire->writable(activeWire->observer, id);
        activeWire->data(activeWire->observer, id, tail.data(), static_cast<uint32_t>(tail.size()));
        activeWire->live = false;
        activeWire->closed(activeWire->observer, id, 0);
        CHECK_TEST(client.send(id, frame) == NI_INVALID_STATE && client.outputState(id) == NI_INVALID_STATE);
        for (unsigned step = 0; step < 16 && !listener.closeCount; ++step) { client.poll(); }
        CHECK_TEST(listener.frames.size() == 7 && listener.frames[5] == 401 && listener.frames[6] == 402 &&
            listener.bodies.size() == 7 && listener.bodies[5] == 1 && listener.bodies[6] == 2 && activeWire->writes == written &&
            listener.closeCount == 1 && listener.closeReason == NET_INTERFACE_TLS_TRUNCATED && !client.peerCount());
        return 0;
    }
    listener.m_rejectFrame = true;
    frame.m_requestId = 401;
    frame.m_payload = body;
    frame.m_payloadSize = sizeof(body);
    CHECK_TEST(server.send(frame) == NI_OK);
    for (uint32_t i = 0; i < 2048 && !listener.m_refusedFrames; ++i)
    {
        CHECK_TEST(advance());
    }
    CHECK_TEST(listener.m_refusedFrames && listener.frames.size() == 5);
    activeWire->live = false;
    activeWire->closed(activeWire->observer, id, 0);
    client.poll();
    const auto begin = PeerOptionNow();
    const auto until = begin + (drainMs == 2 ? 1000 : 5);
    while (!listener.closeCount && PeerOptionNow() < until)
    {
        client.poll();
    }
    if (drainMs != 2)
    {
        CHECK_TEST(!listener.closeCount && listener.frames.size() == 5);
        CHECK_TEST(client.outputState(id) == NI_INVALID_STATE && client.send(id, frame) == NI_INVALID_STATE);
        CHECK_TEST(client.configureCompact(id, true) == NI_INVALID_STATE &&
            client.configureCompression(id, nullptr, 0, false) == NI_INVALID_STATE);
        listener.m_rejectFrame = false;
        for (uint32_t i = 0; i < 8 && !listener.closeCount; ++i)
        {
            client.poll();
        }
        CHECK_TEST(listener.frames.size() == 6 && listener.bodies.size() == 6 && listener.frames.back() == 401 && listener.bodies.back() == body[0]);
    }
    else
    {
        CHECK_TEST(listener.frames.size() == 5);
    }
    CHECK_TEST(listener.closeCount == 1 && listener.closeReason == NET_INTERFACE_TLS_TRUNCATED && client.peerCount() == 0);
    client.shutdown();
    return 0;
}

/** 使用短、长两个排空期限分别驱动相同的直连 TLS 场景
@return 两种期限和原有的帧往返用例均通过时返回0
*/
static int TlsFrames()
{
    CHECK_TEST(CheckTlsFrames(2) == 0 && CheckTlsFrames(60000) == 0);
    return CheckTlsFrames(60000, false, true);
}

/** 检查原始宿主恢复可写后能解密此前积压的完整大帧
@return 全部通过为0
*/
static int LargeTlsWrite()
{
    return CheckTlsFrames(60000, true);
}

/** 通过宿主迟延建连和业务拒收分别验证中继启动、背压时限
@param [in] blocked true表示两条连接均已建立，但业务暂时拒收中继记录
@param [in] remoteEof 是否在恢复接收前触发底层关闭
@return 只有对应期限较短的实例超时，另一条连接可继续推进时返回0
*/
static int CheckRelayDeadline(bool blocked, bool remoteEof = false)
{
    NetRelayPoolConfig limits;
    limits.m_startupMs = blocked ? 60000 : 2;
    limits.m_backpressureMs = blocked ? 2 : 60000;
    PeerTestListener shortListener, longListener;
    NetRelayTransport shortRelay(NetHostPoolConfig(), limits);
    std::swap(limits.m_startupMs, limits.m_backpressureMs);
    NetHostPoolConfig host;
    if (remoteEof) { host.m_eventBudget = 1; }
    NetRelayTransport longRelay(host, limits);
    uint64_t shortId = 0, longId = 0;
    CHECK_TEST(shortRelay.connect("127.0.0.1", 9001, &shortListener, shortId) == NI_OK && activeWire);
    PeerTestWire* shortWire = activeWire;
    CHECK_TEST(longRelay.connect("127.0.0.1", 9002, &longListener, longId) == NI_OK && activeWire && activeWire != shortWire);
    PeerTestWire* longWire = activeWire;
    const uint8_t body[] = {17, 23, 59};
    if (blocked)
    {
        shortWire->connected(shortWire->observer, shortId);
        longWire->connected(longWire->observer, longId);
        CHECK_TEST(shortRelay.poll() && longRelay.poll() && shortListener.connectedCount == 1 && longListener.connectedCount == 1);
        NetInterfaceTunnelRecord record = {};
        record.m_kind = NI_TUNNEL_BYTES;
        record.m_product = 7;
        record.m_channel = 1;
        record.m_data = body;
        record.m_size = sizeof(body);
        uint8_t packet[NET_INTERFACE_TUNNEL_HEADER_SIZE + sizeof(body)] = {};
        uint32_t size = 0;
        CHECK_TEST(NetInterfaceTunnel::encode(record, packet, sizeof(packet), size) == NI_OK && size == sizeof(packet));
        shortListener.m_rejectRelayRecord = longListener.m_rejectRelayRecord = true;
        shortWire->data(shortWire->observer, shortId, packet, size);
        longWire->data(longWire->observer, longId, packet, size);
    }
    const auto guard = PeerOptionNow() + UINT64_C(2000);
    while (shortListener.closeCount == 0 && PeerOptionNow() < guard)
    {
        shortRelay.poll();
        CHECK_TEST(longRelay.poll());
    }
    CHECK_TEST(shortListener.closeCount == 1 && shortListener.closeReason == NI_TIMEOUT && longListener.closeCount == 0);
    if (blocked)
    {
        CHECK_TEST(shortListener.m_relayOffers > 0 && longListener.m_relayOffers > 0 && longListener.m_relayBody.empty());
        if (remoteEof)
        {
            NetInterfaceTunnelRecord pending = {};
            pending.m_kind = NI_TUNNEL_BYTES;
            pending.m_product = 7;
            pending.m_channel = 1;
            pending.m_data = body;
            pending.m_size = sizeof(body);
            CHECK_TEST(longRelay.send(longId, pending) == NI_OK && longRelay.outputState(longId) == NI_READY);
            longWire->writable(longWire->observer, longId);
            longWire->live = false;
            longWire->closed(longWire->observer, longId, -83);
            CHECK_TEST(longRelay.poll() && !longListener.closeCount && longListener.m_relayBody.empty());
            CHECK_TEST(longRelay.outputState(longId) == NI_INVALID_STATE && longRelay.send(longId, pending) == NI_INVALID_STATE);
            longListener.m_rejectRelayRecord = false;
            longRelay.poll();
            CHECK_TEST(longListener.closeCount == 1 && longListener.closeReason == NI_INVALID_STATE &&
                longListener.m_relayBody.size() == sizeof(body) && std::memcmp(longListener.m_relayBody.data(), body, sizeof(body)) == 0);
            longRelay.shutdown();
            return 0;
        }
        longListener.m_rejectRelayRecord = false;
        CHECK_TEST(longRelay.poll() && !longWire->paused && longListener.m_relayBody.size() == sizeof(body));
        CHECK_TEST(std::memcmp(longListener.m_relayBody.data(), body, sizeof(body)) == 0);
    }
    else
    {
        CHECK_TEST(shortListener.connectedCount == 0 && longListener.connectedCount == 0);
        longWire->connected(longWire->observer, longId);
        CHECK_TEST(longRelay.poll() && longListener.connectedCount == 1);
    }
    shortRelay.shutdown();
    longRelay.shutdown();
    CHECK_TEST(shortListener.closeCount == 1 && longListener.closeCount == 1 && longListener.closeReason == NI_OK);
    return 0;
}

/** 设置中继的四个物理池，用真正的记录写入分别观察控制和数据分配。
@return 全部通过为0
*/
static int RelayPools()
{
    CHECK_TEST(InstallWire() == NI_OK);
    size_t counts[4] = {};
    NetRelayPoolConfig pools;
    pools.m_stepBudget = 1;
    ObservePool(pools.m_dataRecords, counts[0]);
    ObservePool(pools.m_dataBytes.m_classes[0], counts[1]);
    ObservePool(pools.m_controlRecords, counts[2]);
    ObservePool(pools.m_controlBytes.m_classes[0], counts[3]);
    PeerTestListener listener;
    NetRelayTransport relay;
    CHECK_TEST(relay.configurePools(pools) == NI_OK);
    uint32_t* budgets[] = {&pools.m_stepBudget, &pools.m_startupMs, &pools.m_backpressureMs};
    for (auto budget : budgets)
    {
        const uint32_t original = *budget;
        *budget = 0;
        CHECK_TEST(!pools.valid() && relay.configurePools(pools) == NI_INVALID_ARGUMENT);
        NetRelayTransport rejected(NetHostPoolConfig(), pools);
        uint64_t unused = 87;
        CHECK_TEST(rejected.connect("127.0.0.1", 9191, &listener, unused) == NI_INVALID_ARGUMENT && unused == 0);
        CHECK_TEST(rejected.listen("127.0.0.1", 9191, &listener, 1) == NI_INVALID_ARGUMENT);
        *budget = original;
    }
    uint64_t id = 0;
    CHECK_TEST(relay.connect("127.0.0.1", 9191, &listener, id) == NI_OK && activeWire);
    activeWire->connected(activeWire->observer, id);
    CHECK_TEST(relay.poll() && listener.connectedCount == 1 && relay.configurePools(pools) == NI_INVALID_STATE);
    activeWire->writable(activeWire->observer, id);
    CHECK_TEST(relay.poll() && listener.connectedCount == 1 && listener.closeCount == 0);
    uint8_t bytes[] = {9,8};
    NetInterfaceTunnelRecord record = {};
    record.m_kind = NI_TUNNEL_BYTES; record.m_product = 7; record.m_channel = 1;
    record.m_data = bytes; record.m_size = sizeof(bytes);
    CHECK_TEST(relay.send(id, record) == NI_OK && counts[0] == 1 && counts[1] == 1 && !counts[2] && !counts[3]);
    record.m_kind = NI_TUNNEL_CONTROL; record.m_product = 0; record.m_channel = 0;
    CHECK_TEST(relay.send(id, record) == NI_OK && counts[2] == 1 && counts[3] == 1);
    const uint32_t one = NET_INTERFACE_TUNNEL_HEADER_SIZE + sizeof(bytes);
    CHECK_TEST(relay.poll() && activeWire->writes == one && relay.poll() && activeWire->writes == 2 * one);
    relay.shutdown();
    size_t growth = 0;
    LockFreeHashMapPoolConfig peerPool;
    ObservePool(peerPool, growth);
    NetLinkTransport link;
    CHECK_TEST(link.configurePeerPool(peerPool) == NI_OK);
    NetInterfaceLinkOptions options = {};
    options.m_version = 3;
    options.m_role = NI_LINK_CLIENT;
    CHECK_TEST(link.connect("127.0.0.1", 9192, options, &listener, id) == NI_OK && growth == 1);
    CHECK_TEST(link.configurePeerPool(peerPool) == NI_INVALID_STATE);
    link.shutdown();
    CHECK_TEST(CheckRelayDeadline(false) == 0);
    CHECK_TEST(CheckRelayDeadline(true) == 0);
    return CheckRelayDeadline(true, true);
}

/** 检查写完成长度不符时的关闭、首个错误及确认边界
@return 全部场景通过时返回0
*/
static int WriteLength()
{
    CHECK_TEST(InstallFileBackend() == 0);
    for (uint32_t scenario = 0; scenario < 4; ++scenario)
    {
        const uint32_t failedSegment = scenario / 2;
        const uint64_t length = scenario % 2 == 0 ? 65535 : 65537;
        NetFileIo io;
        const NetFileIo& view = io;
        CHECK_TEST(view.released() && view.done() && !view.accepting() && view.status() == NI_OK);
        rejectNextFileFactory = true;
        CHECK_TEST(io.init() == NI_NO_MEMORY && view.released() && view.done() && view.status() == NI_NO_MEMORY);
        CHECK_TEST(io.init() == NI_OK && view.accepting() && !view.done());
        NetInterfaceFileInfo info = {};
        info.direction = 1; info.state = NI_FILE_TRANSFERRING;
        info.chunk = 65536; info.window = 2; info.total = 131072;
        info.stream = 61; info.expiresUtc = 3600;
        info.transfer[0] = 1; info.token[0] = 2; info.hash[0] = 3;
        NetFileTransfer receiver;
        CHECK_TEST(receiver.setWriteBatchChunks(2) == NI_OK &&
            receiver.start(io, 1, info, 7, false, nullptr, nullptr, 100) == NI_OK);
        NetInterfaceFileFlow source(info, 7, true, 100);
        std::vector<uint8_t> payload(65536, 23);
        NetInterfaceFrame frame = {};
        for (unsigned chunk = 0; chunk < 2; ++chunk)
        {
            uint64_t offset = 0;
            uint32_t size = 0;
            CHECK_TEST(source.readRequest(offset, size) == NI_READY && offset == chunk * 65536 && size == payload.size());
            CHECK_TEST(source.produced(offset, payload.data(), size) == NI_OK && source.peekOutput(frame) == NI_READY &&
                receiver.accept(frame) == NI_OK && source.consumeOutput() == NI_OK && receiver.poll(101 + chunk) == NI_OK);
        }
        NetFileIoResult result = {};
        CHECK_TEST(io.peek(result) == NI_READY && result.operation == NET_FILE_IO_WRITE && result.length == 65536);
        if (failedSegment == 1)
        {
            CHECK_TEST(receiver.onIoResult(result) && io.consume(result.id) == NI_OK && receiver.poll(103) == NI_OK);
            CHECK_TEST(receiver.peekOutput(frame) == NI_NEED_MORE && io.peek(result) == NI_READY &&
                result.operation == NET_FILE_IO_WRITE && result.offset == 65536 && result.length == 65536);
        }
        fileBackend->completion.length = length;
        CHECK_TEST(io.peek(result) == NI_READY && receiver.onIoResult(result) && io.consume(result.id) == NI_OK);
        int32_t failure = NI_OK;
        receiver.snapshot(info, failure);
        NetInterfaceFileAck ack = {};
        CHECK_TEST(failure == NI_INVALID_STATE && receiver.peekOutput(frame) == NI_READY && frame.m_kind == NI_DATA_ACK);
        CHECK_TEST(NetInterfaceFile::parseAck(frame, ack) == NI_OK && ack.flags == 2 && ack.result == failure && ack.offset == 0);
        CHECK_TEST(source.accept(frame) == failure && source.progress(104) == failure);
        CHECK_TEST(receiver.consumeOutput() == NI_OK);
        CHECK_TEST(receiver.poll(104) == NI_INVALID_STATE && io.peek(result) == NI_READY && result.operation == NET_FILE_IO_CLOSE);
        CHECK_TEST(receiver.onIoResult(result) && io.consume(result.id) == NI_OK && receiver.poll(105) == NI_INVALID_STATE);
        CHECK_TEST(receiver.done() && fileBackend->next == failedSegment + 2 && io.peek(result) == NI_NEED_MORE);
        CHECK_TEST(fileBackend->written.size() == (failedSegment + 1) * payload.size());
        io.stop();
        io.uninit();
        CHECK_TEST(view.released() && view.done());
    }
    return 0;
}

/** 通过公开 END/ACK 交换验证协议成功后本地关文件的独立结果
@return 全部通过为0
*/
static int SenderFileClose()
{
    CHECK_TEST(InstallFileBackend() == 0);
    NetFileIo io;
    CHECK_TEST(io.init() == NI_OK && fileBackend);
    NetInterfaceFileInfo info = {};
    info.direction = 1;
    info.state = NI_FILE_TRANSFERRING;
    info.chunk = 1024;
    info.window = 1;
    info.stream = 61;
    info.expiresUtc = 3600;
    info.token[0] = info.transfer[0] = 1;
    const uint8_t empty = 0;
    CHECK_TEST(NetInterface::sha256(&empty, 0, info.hash, sizeof(info.hash)) == NI_OK);
    const int32_t outcomes[] = { NI_OK, NI_TIMEOUT };
    for (unsigned scenario = 0; scenario < 6; ++scenario)
    {
        const int32_t outcome = outcomes[scenario % 2];
        const bool stopping = scenario >= 2;
        NetFileTransfer sender;
        NetInterfaceFileFlow receiver(info, 7, false, 100);
        CHECK_TEST(sender.start(io, 31, info, 7, true, nullptr, nullptr, 100) == NI_OK);
        NetInterfaceFrame frame = {};
        CHECK_TEST(sender.peekOutput(frame) == NI_READY && receiver.accept(frame) == NI_OK && sender.consumeOutput() == NI_OK);
        CHECK_TEST(receiver.takeCommit() == NI_READY && receiver.committed(NI_OK) == NI_READY);
        CHECK_TEST(receiver.peekOutput(frame) == NI_READY && sender.accept(frame) == NI_OK);
        if (scenario >= 4)
        {
            // 分别在第一次 CLOSE 尝试之前、或共享队列拒收 CLOSE 之后停止后端。
            NetFileIoResult completion = {};
            if (scenario == 5)
            {
                NetFileIoRequest other = {};
                other.operation = NET_FILE_IO_SIZE;
                other.file = 32;
                CHECK_TEST(io.nextRequestId(other.id) == NI_OK && io.submit(other) == NI_OK);
                CHECK_TEST(sender.poll(101) == NI_NEED_MORE && !sender.done() && io.peek(completion) == NI_READY &&
                    completion.id == other.id && !sender.onIoResult(completion) && io.consume(other.id) == NI_OK);
            }
            const uint64_t beforeStop = fileBackend->next;
            io.stop();
            CHECK_TEST(io.done() && io.peek(completion) == NI_NEED_MORE && sender.poll(102) == NI_CANCELLED && sender.done());
            NetInterfaceFileInfo snapshot = {};
            int32_t local = NI_OK;
            sender.snapshot(snapshot, local);
            CHECK_TEST(snapshot.state == NI_FILE_COMPLETED && local == NI_CANCELLED && sender.poll(103) == NI_CANCELLED &&
                fileBackend->next == beforeStop && io.peek(completion) == NI_NEED_MORE && receiver.consumeOutput() == NI_OK);
            io.uninit();
            CHECK_TEST(io.released() && io.init() == NI_OK);
            continue;
        }
        const uint64_t before = fileBackend->next;
        CHECK_TEST(sender.poll(101) == NI_OK && !sender.done());
        fileBackend->completion.result = outcome;
        if (stopping)
        {
            io.stop();
            CHECK_TEST(io.done() && sender.poll(102) == NI_NEED_MORE && !sender.done());
        }
        NetFileIoResult closed = {};
        CHECK_TEST(io.peek(closed) == NI_READY && closed.operation == NET_FILE_IO_CLOSE && closed.file == 31);
        CHECK_TEST(sender.onIoResult(closed) && io.consume(closed.id) == NI_OK);
        const int32_t expected = outcome == NI_OK ? NI_READY : outcome;
        CHECK_TEST(sender.poll(102) == expected && sender.done());
        // 接收端仍持有原 ACK。发送端收敛后再次收到该帧，保留原来的本地关闭结果。
        CHECK_TEST(sender.accept(frame) == NI_NOT_FOUND && receiver.consumeOutput() == NI_OK);
        sender.cancel();
        NetInterfaceFileInfo snapshot = {};
        int32_t local = NI_NEED_MORE;
        sender.snapshot(snapshot, local);
        CHECK_TEST(snapshot.state == NI_FILE_COMPLETED && local == expected && sender.poll(103) == expected && fileBackend->next == before + 1);
        if (stopping)
        {
            io.uninit();
            CHECK_TEST(io.released() && io.init() == NI_OK);
        }
    }
    // 恢复前缀期间拒收，摘要补齐后重投同一帧。终态的 -3 不再阻塞后续派发。
    info.offset = info.total = 1;
    fileBackend->cache = true;
    fileBackend->saved.assign(1, empty);
    CHECK_TEST(NetInterface::sha256(&empty, 1, info.hash, sizeof(info.hash)) == NI_OK);
    const int32_t reasons[] = { NI_LIMIT_EXCEEDED, -83, NI_LIMIT_EXCEEDED, NI_LIMIT_EXCEEDED, NI_LIMIT_EXCEEDED };
    for (uint32_t scenario = 0; scenario < sizeof(reasons) / sizeof(reasons[0]); ++scenario)
    {
        const bool reading = scenario == 4;
        info.offset = reading ? 0 : info.total;
        NetFileTransfer sender;
        CHECK_TEST(sender.start(io, 76, info, 7, true, nullptr, nullptr, 100) == NI_OK);
        NetInterfaceFileAck ack = {};
        ack.flags = 2;
        ack.result = reasons[scenario];
        ack.offset = scenario == 3 ? 1024 : info.offset;
        std::memcpy(ack.token, info.token, sizeof(ack.token));
        if (scenario == 2) { ++ack.token[0]; }
        uint8_t payload[NET_INTERFACE_FILE_ACK_SIZE] = {};
        CHECK_TEST(NetInterfaceFile::encodeAck(ack, payload, sizeof(payload)) == NI_OK);
        NetInterfaceFrame frame = {};
        frame.m_kind = NI_DATA_ACK;
        frame.m_productId = 7;
        frame.m_requestId = info.stream;
        frame.m_messageType = NI_FILE_DATA_TYPE;
        frame.m_payload = payload;
        frame.m_payloadSize = sizeof(payload);
        if (!reading)
        {
            CHECK_TEST(sender.accept(frame) == NI_LIMIT_EXCEEDED);
        }
        NetFileIoResult read = {};
        CHECK_TEST(sender.poll(100) == NI_OK && io.peek(read) == NI_READY && read.operation == NET_FILE_IO_READ &&
            read.file == 76 && read.size == 1);
        if (!reading)
        {
            CHECK_TEST(sender.onIoResult(read) && io.consume(read.id) == NI_OK);
        }
        const int32_t failure = scenario == 2 || scenario == 3 ? NI_PROTOCOL_ERROR : reasons[scenario];
        CHECK_TEST(sender.accept(frame) == (failure == NI_LIMIT_EXCEEDED ? NI_OK : failure));
        if (failure == NI_LIMIT_EXCEEDED)
        {
            CHECK_TEST(sender.accept(frame) == NI_NOT_FOUND);
        }
        NetInterfaceFileInfo snapshot = {};
        int32_t result = NI_OK;
        sender.snapshot(snapshot, result);
        CHECK_TEST(result == failure && snapshot.state == NI_FILE_FAILED);
        if (reading)
        {
            // 正文 READ 已接纳但尚未分派，协议先保存失败，宿主仍须路由该任务。
            CHECK_TEST(sender.poll(101) == NI_NEED_MORE && !sender.done());
            sender.snapshot(snapshot, result);
            CHECK_TEST(result == failure && snapshot.state == NI_FILE_FAILED &&
                sender.onIoResult(read) && io.consume(read.id) == NI_OK);
            NetFileIoRequest other = {};
            other.operation = NET_FILE_IO_SIZE;
            other.file = 77;
            CHECK_TEST(io.nextRequestId(other.id) == NI_OK && io.submit(other) == NI_OK);
            NetFileIoResult occupied = {};
            CHECK_TEST(sender.poll(101) == NI_NEED_MORE && !sender.done() && io.peek(occupied) == NI_READY &&
                occupied.id == other.id && !sender.onIoResult(occupied) && io.consume(other.id) == NI_OK);
        }
        const uint64_t before = fileBackend->next;
        NetFileIoResult closed = {};
        CHECK_TEST(sender.poll(101) == failure && io.peek(closed) == NI_READY && closed.operation == NET_FILE_IO_CLOSE &&
            closed.file == 76 && sender.poll(102) == NI_NEED_MORE && !sender.done());
        sender.snapshot(snapshot, result);
        CHECK_TEST(result == failure && snapshot.state == NI_FILE_FAILED && sender.onIoResult(closed) && io.consume(closed.id) == NI_OK);
        CHECK_TEST(sender.poll(102) == failure && sender.done() && fileBackend->next == before + 1);
        if (failure == NI_LIMIT_EXCEEDED)
        {
            CHECK_TEST(sender.accept(frame) == NI_NOT_FOUND);
            sender.snapshot(snapshot, result);
            CHECK_TEST(snapshot.state == NI_FILE_FAILED && result == NI_LIMIT_EXCEEDED && sender.poll(103) == result);
        }
    }
    return 0;
}

/** 验证共享文件后端上的三个接收实例分别使用其初始进度时限
@return 全部实例仅在各自期限关闭对应文件时返回0
*/
static int FileProgressTimeout()
{
    CHECK_TEST(InstallFileBackend() == 0);
    NetFileIo io;
    CHECK_TEST(io.init() == NI_OK);
    NetInterfaceFileInfo info = {};
    info.direction = 1;
    info.state = NI_FILE_TRANSFERRING;
    info.chunk = 1024;
    info.window = 1;
    info.total = 1024;
    info.expiresUtc = 3600;
    info.token[0] = info.transfer[0] = info.hash[0] = 1;
    NetFileTransfer flows[3];
    CHECK_TEST(flows[0].setProgressTimeout(1000) == NI_OK && flows[2].setProgressTimeout(60000) == NI_OK);
    const uint64_t start = 100;
    const uint32_t limits[] = {1000, 15000, 60000};
    for (uint32_t i = 0; i < 3; ++i)
    {
        info.stream = 41 + i;
        CHECK_TEST(flows[i].start(io, info.stream, info, 7, false, nullptr, nullptr, start) == NI_OK);
    }
    for (uint32_t i = 0; i < 3; ++i)
    {
        const uint64_t deadline = start + limits[i];
        for (uint32_t j = i; j < 3; ++j)
        {
            CHECK_TEST(flows[j].poll(deadline - 1) == NI_OK);
        }
        NetFileIoResult close = {};
        CHECK_TEST(io.peek(close) == NI_NEED_MORE && flows[i].poll(deadline) == NI_TIMEOUT);
        CHECK_TEST(io.peek(close) == NI_READY && close.operation == NET_FILE_IO_CLOSE && close.file == 41 + i);
        CHECK_TEST(flows[i].onIoResult(close) && io.consume(close.id) == NI_OK);
        for (uint32_t j = i + 1; j < 3; ++j)
        {
            CHECK_TEST(flows[j].poll(deadline) == NI_OK && !flows[j].done());
        }
        CHECK_TEST(flows[i].poll(deadline) == NI_TIMEOUT && flows[i].done() && io.peek(close) == NI_NEED_MORE);
    }
    return CheckFinalFileOutput(nullptr, nullptr);
}

/** 检查不同缓存实例的发布间隔和写完成的文件身份
@return 全部通过为0
*/
static int CacheWrites()
{
    CHECK_TEST(InstallFileBackend() == 0 && InstallWire() == NI_OK);
    NetInterfaceDirectoryNodeData point = {};
    point.m_structSize = sizeof(point);
    point.m_version = 1;
    std::memcpy(point.m_name, "node.0123456789abcdef0123456789abcdef", sizeof("node.0123456789abcdef0123456789abcdef") - 1);
    std::memcpy(point.m_address, "127.0.0.1", sizeof("127.0.0.1"));
    point.m_port = 9000;
    point.m_capacity = 32;
    point.m_boot = 9;
    NetGatewayDirectoryStore reopening;
    CHECK_TEST(reopening.open("peer.cache") == NI_OK);
    fileBackend->rejectNextRelease = true;
    reopening.shutdown();
    CHECK_TEST(reopening.poll() == NI_INVALID_STATE && !reopening.stopped());
    CHECK_TEST(reopening.open("second.cache") == NI_INVALID_STATE);
    reopening.poll();
    CHECK_TEST(reopening.stopped() && reopening.open("second.cache") == NI_OK);
    reopening.shutdown();
    CHECK_TEST(reopening.poll() == NI_OK && reopening.stopped());
    for (unsigned mode = 0; mode < 9; ++mode)
    {
        NetGatewayDirectoryStore store(mode == 1 ? UINT32_MAX : 0);
        CHECK_TEST(store.open("peer.cache") == NI_OK && fileBackend);
        fileBackend->cache = true;
        CHECK_TEST(store.queue(&point, 1) == NI_OK);
        CHECK_TEST(store.poll() == NI_OK && fileBackend->completion.operation == NET_FILE_IO_OPEN_READ);
        if (mode == 4 || mode == 5)
        {
            fileBackend->completion.result = NI_OK;
            fileBackend->completion.file = 77;
            fileBackend->completion.length = 48;
            CHECK_TEST(store.poll() == NI_OK && fileBackend->completion.operation == NET_FILE_IO_READ);
            if (mode == 4) { ++fileBackend->completion.file; }
            else { ++fileBackend->completion.offset; }
            CHECK_TEST(store.poll() == NI_PROTOCOL_ERROR && fileBackend->stopped && fileBackend->published == 0);
            store.poll();
            CHECK_TEST(store.stopped());
            continue;
        }
        store.poll();
        CHECK_TEST(fileBackend->completion.operation == NET_FILE_IO_OPEN_TEMPORARY);
        store.poll();
        CHECK_TEST(fileBackend->completion.operation == NET_FILE_IO_WRITE && fileBackend->completion.file == 77);
        if (mode == 2 || mode == 3)
        {
            if (mode == 2) { ++fileBackend->completion.file; }
            else { ++fileBackend->completion.offset; }
            CHECK_TEST(store.poll() == NI_PROTOCOL_ERROR && fileBackend->published == 0);
        }
        else if (mode == 8)
        {
            fileBackend->completion.result = NI_TIMEOUT;
            CHECK_TEST(store.poll() == NI_TIMEOUT && fileBackend->completion.operation == NET_FILE_IO_CLOSE);
            ++fileBackend->completion.file;
            CHECK_TEST(store.poll() == NI_PROTOCOL_ERROR && fileBackend->stopped && fileBackend->published == 0);
        }
        else if (mode >= 6)
        {
            CHECK_TEST(store.poll() == NI_OK && fileBackend->completion.operation == NET_FILE_IO_PUBLISH && fileBackend->published == 0);
            if (mode == 6)
            {
                fileBackend->completion.result = NI_TIMEOUT;
                CHECK_TEST(store.poll() == NI_TIMEOUT && fileBackend->published == 0);
            }
            else
            {
                ++fileBackend->completion.file;
                CHECK_TEST(store.poll() == NI_PROTOCOL_ERROR && fileBackend->stopped && fileBackend->published == 0);
            }
        }
        else
        {
            CHECK_TEST(store.poll() == NI_OK && fileBackend->completion.operation == NET_FILE_IO_PUBLISH && fileBackend->published == 0);
            CHECK_TEST(store.poll() == NI_OK && fileBackend->published == 1);
            point.m_port++;
            CHECK_TEST(store.queue(&point, 1) == NI_OK);
            for (unsigned step = 0; step < 4; ++step) { CHECK_TEST(store.poll() == NI_OK); }
            CHECK_TEST(fileBackend->published == (mode == 1 ? 1U : 2U));
        }
        store.shutdown();
        store.poll();
        CHECK_TEST(store.stopped());
        store.shutdown();
        CHECK_TEST(store.stopped() && store.open("reopen") == NI_OK);
        store.shutdown();
        store.poll();
        CHECK_TEST(store.stopped());
    }
    return 0;
}

/** 通过写出并重新加载验证目录的真实编码、节点上限及快照所有权
@return 全部通过为0
*/
static int CacheRoundTrip()
{
    CHECK_TEST(InstallFileBackend() == 0 && InstallWire() == NI_OK);
    NetGatewayDirectoryStore store(0);
    store.shutdown();
    CHECK_TEST(store.stopped());
    CHECK_TEST(store.open(std::string(4059, 'c')) == NI_OK && fileBackend);
    fileBackend->cache = true;
    std::vector<NetInterfaceDirectoryPoint> points(65);
    const char digits[] = "0123456789abcdef";
    for (size_t i = 0; i < points.size(); ++i)
    {
        NetInterfaceDirectoryPoint& point = points[i];
        point.m_name = "node.0123456789abcdef0123456789abcdef";
        point.m_name[35] = digits[i >> 4];
        point.m_name[36] = digits[i & 15];
        point.m_address = "127.0.0.1";
        point.m_port = static_cast<uint32_t>(8010 + i);
        point.m_capacity = static_cast<uint32_t>(i + 1);
        point.m_boot = i + 3;
    }
    CHECK_TEST(store.queue(points) == NI_OK);
    const size_t excessive = NET_INTERFACE_DIRECTORY_BYTES / NET_INTERFACE_DIRECTORY_NAME_BYTES + 1;
    std::vector<NetInterfaceDirectoryPoint> rejected(excessive);
    rejected[0].m_address.assign(512, 'x');
    CHECK_TEST(store.queue(rejected) == NI_LIMIT_EXCEEDED);
    for (unsigned i = 0; i < 8 && !fileBackend->published; ++i)
    {
        store.poll();
    }
    CHECK_TEST(fileBackend->published == 1 && !fileBackend->saved.empty());
    std::vector<uint8_t> disk(fileBackend->saved);
    store.shutdown();
    store.poll();
    CHECK_TEST(store.stopped() && store.open("peer.cache") == NI_OK && fileBackend);
    fileBackend->cache = true;
    fileBackend->saved.swap(disk);
    for (unsigned i = 0; i < 4; ++i)
    {
        CHECK_TEST(store.poll() == NI_OK);
    }
    uint32_t count = 0;
    CHECK_TEST(store.takeLoaded(nullptr, 0, count) == NI_BUFFER_TOO_SMALL && count == points.size());
    std::vector<NetInterfaceDirectoryNodeData> output(count);
    for (auto& node : output)
    {
        node.m_version = 1;
        node.m_structSize = sizeof(node);
    }
    CHECK_TEST(store.takeLoaded(output.data(), count, count) == NI_READY && count == points.size());
    for (size_t i = 0; i < points.size(); ++i)
    {
        CHECK_TEST(points[i].m_name == output[i].m_name && points[i].m_address == output[i].m_address &&
            points[i].m_port == output[i].m_port && points[i].m_capacity == output[i].m_capacity && points[i].m_boot == output[i].m_boot);
    }
    CHECK_TEST(store.takeLoaded(nullptr, 0, count) == NI_NEED_MORE && count == 0);
    disk.swap(fileBackend->saved);
    store.shutdown();
    store.poll();
    const size_t readOnlyLengths[] = {4060, 4096};
    for (auto length : readOnlyLengths)
    {
        CHECK_TEST(store.stopped() && store.open(std::string(length, 'c')) == NI_OK);
        fileBackend->cache = true;
        fileBackend->saved = disk;
        CHECK_TEST(store.queue(points) == NI_INVALID_ARGUMENT && store.queue(output.data(), 1) == NI_INVALID_ARGUMENT);
        for (unsigned i = 0; i < 4; ++i) { CHECK_TEST(store.poll() == NI_OK); }
        CHECK_TEST(store.takeLoaded(output.data(), static_cast<uint32_t>(output.size()), count) == NI_READY &&
            count == points.size() && output[0].m_boot == points[0].m_boot && !fileBackend->published);
        store.shutdown();
        store.poll();
    }
    CHECK_TEST(store.stopped());
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
    bool correct = true;
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
            correct = false;
        }
        return valid;
    });
    CHECK_TEST(correct);
    return 0;
}

/** 向对端交付一段已捕获的真实字节，遵守对端的暂停读取要求。
@param [in,out] from 发送端字节宿主
@param [in,out] to 接收端字节宿主
*/
static void TransferDiscoveryBytes(PeerTestWire& from, PeerTestWire& to)
{
    if (!to.paused && !from.output.empty())
    {
        const size_t size = (std::min)(from.output.size(), static_cast<size_t>(1024));
        to.data(to.observer, 1, from.output.data(), static_cast<uint32_t>(size));
        from.output.erase(from.output.begin(), from.output.begin() + size);
    }
}

/** 通过已认证的真实管理通道分别观察查询应答超时与定时刷新。
@param [in] node 测试节点的独立叶身份，借用至返回
@param [in] reply 是否对首条查询返回合法的单节点目录
@param [in] shortDeadline 非慢应答场景中，true 使用短期限，false 使用60000毫秒
@param [in] holdRequest 是否由监听者拒收首个已认证的管理请求，检验接收背压期限
@param [in] slowReply 仅与 reply 配合使用，延迟120毫秒后交付合法目录，并按单调时钟观察刷新和下一条查询超时
@return 观察到预期查询次数、响应和关闭行为时返回0
*/
static int CheckDiscoveryTimer(const NetInterfaceLinkOptions& node, bool reply, bool shortDeadline, bool holdRequest = false,
    bool slowReply = false)
{
    PeerTestListener listener;
    if (holdRequest) { listener.m_acceptControlThrough = 0; }
    NetLinkTransportConfig serverConfig;
    if (holdRequest) { serverConfig.m_backpressureMs = shortDeadline ? 10 : 60000; }
    NetLinkTransport server(serverConfig);
    CHECK_TEST(server.listen("127.0.0.1", 9500, node, &listener) == NI_OK && activeWire);
    PeerTestWire& serverWire = *activeWire;
    NetGatewayDiscoveryConfig config;
    config.m_responseMs = slowReply ? 500 : (!holdRequest && !reply && shortDeadline ? 100 : 60000);
    config.m_refreshMs = slowReply ? 100 : (reply && shortDeadline ? 10 : 60000);
    config.m_retryMs = 60000;
    int32_t closeReason = NI_READY;
    NetGatewayDiscovery probe(config);
    std::vector<NetGatewayAddress> seeds;
    CHECK_TEST(ParseTestSeeds("127.0.0.1:9500", seeds) == NI_OK && probe.start(seeds) == NI_OK);
    CHECK_TEST(probe.poll() && activeWire && activeWire != &serverWire);
    activeWire->m_closeReason = &closeReason;
    serverWire.capture = serverWire.live = activeWire->capture = true;
    serverWire.connected(serverWire.observer, 1);
    activeWire->connected(activeWire->observer, 1);
    // 双方各推进一次再交换密文。返回 false 表示发现器或监听端已经停止。
    const auto advance = [&]() -> bool {
        if (!probe.poll() || !server.poll())
        {
            return false;
        }
        if (activeWire)
        {
            TransferDiscoveryBytes(*activeWire, serverWire);
            TransferDiscoveryBytes(serverWire, *activeWire);
        }
        return true;
    };
    const auto handshakeEnd = PeerOptionNow() + UINT64_C(5000);
    while (listener.frames.empty() && !listener.m_refusedControl && activeWire && PeerOptionNow() < handshakeEnd)
    {
        CHECK_TEST(advance());
    }
    if (holdRequest)
    {
        CHECK_TEST(listener.readyCount == 1 && listener.frames.empty() && listener.m_refusedControl > 0);
        const auto until = PeerOptionNow() + (shortDeadline ? 2000 : 150);
        while (!listener.closeCount && PeerOptionNow() < until) { CHECK_TEST(advance()); }
        if (shortDeadline)
        {
            CHECK_TEST(listener.closeCount == 1 && listener.closeReason == NI_TIMEOUT && listener.frames.empty());
        }
        else
        {
            CHECK_TEST(!listener.closeCount && listener.frames.empty());
            listener.m_acceptControlThrough = UINT64_MAX;
            CHECK_TEST(advance() && advance() && listener.frames.size() == 1 && listener.frames[0] == 1);
        }
        return 0;
    }
    CHECK_TEST(activeWire && listener.frames.size() == 1 && listener.frames[0] == 1);
    if (reply)
    {
        NetInterfaceDirectoryNodeData point = {};
        point.m_structSize = sizeof(point);
        point.m_version = 1;
        CHECK_TEST(node.m_name && std::strlen(node.m_name) < sizeof(point.m_name));
        std::memcpy(point.m_name, node.m_name, std::strlen(node.m_name) + 1);
        std::memcpy(point.m_address, "127.0.0.1", sizeof("127.0.0.1"));
        point.m_port = 9500;
        point.m_capacity = 5;
        point.m_boot = 7;
        NetInterfaceDirectoryHandle directory = nullptr;
        CHECK_TEST(NetInterface::directoryCreateNodes(&point, 1, &directory) == NI_OK && directory);
        // NGC3 由 16 字节头、发现正文和 32 字节摘要组成，测试程序持有整个缓冲区。
        uint8_t cache[NET_INTERFACE_DIRECTORY_BYTES + 48] = {};
        uint32_t size = 0;
        const int32_t encoded = NetInterface::directoryCache(directory, 1, 0, cache, sizeof(cache), &size);
        NetInterface::directoryDestroy(directory);
        CHECK_TEST(encoded == NI_OK && size > 48 && size <= sizeof(cache));
        NetInterfaceFrame response = {};
        response.m_kind = NI_RESPONSE;
        response.m_messageType = NI_GATEWAY_DISCOVER;
        response.m_requestId = 1;
        response.m_payload = cache + 16;
        response.m_payloadSize = size - 48;
        if (slowReply)
        {
            // 第一条查询已经到达节点。持续驱动两端，跨过一次刷新间隔再发送应答。
            const uint64_t replyAt = PeerOptionNow() + 120;
            while (PeerOptionNow() < replyAt)
            {
                CHECK_TEST(advance() && activeWire && closeReason == NI_READY &&
                    listener.frames.size() == 1 && probe.discovered() == 0);
            }
        }
        CHECK_TEST(server.sendControl(1, response) == NI_OK);
    }
    if (slowReply)
    {
        // 在发现器公开候选表的那次 poll 前取样，作为响应起算时刻的下界。
        uint64_t responsePollStart = 0;
        const uint64_t deliveryLimit = PeerOptionNow() + 3000;
        while (probe.discovered() == 0 && PeerOptionNow() < deliveryLimit)
        {
            responsePollStart = PeerOptionNow();
            CHECK_TEST(advance() && activeWire && closeReason == NI_READY && listener.frames.size() == 1);
        }
        CHECK_TEST(probe.discovered() == 1);
        const uint64_t earliestRefresh = responsePollStart + config.m_refreshMs;
        while (listener.frames.size() == 1 && PeerOptionNow() < earliestRefresh + 3000)
        {
            CHECK_TEST(advance() && activeWire && closeReason == NI_READY);
        }
        const uint64_t secondReceived = PeerOptionNow();
        CHECK_TEST(listener.frames.size() == 2 && listener.frames[1] == 2 && secondReceived + 2 >= earliestRefresh);
        // 不应答第二条查询。跨过刷新间隔时应仍活动，随后只能收到该查询的应答超时。
        while (PeerOptionNow() < secondReceived + 120)
        {
            CHECK_TEST(advance() && activeWire && closeReason == NI_READY && listener.frames.size() == 2);
        }
        const uint64_t closeLimit = secondReceived + config.m_responseMs + 3000;
        while (activeWire && PeerOptionNow() < closeLimit)
        {
            CHECK_TEST(advance());
        }
        CHECK_TEST(!activeWire && closeReason == NI_TIMEOUT && listener.frames.size() == 2 && probe.discovered() == 1 &&
            PeerOptionNow() + 2 >= earliestRefresh + config.m_responseMs);
        return 0;
    }
    const auto until = PeerOptionNow() + (shortDeadline ? 2000 : 150);
    while (activeWire && listener.frames.size() == 1 && PeerOptionNow() < until)
    {
        CHECK_TEST(advance());
    }
    CHECK_TEST(probe.discovered() == (reply ? 1U : 0U));
    CHECK_TEST((activeWire == nullptr) == (!reply && shortDeadline));
    CHECK_TEST(closeReason == (!reply && shortDeadline ? NI_TIMEOUT : NI_READY));
    if (reply && shortDeadline)
    {
        CHECK_TEST(listener.frames.size() == 2 && listener.frames[1] == 2);
        // 有效控制帧中的空目录正文必须按协议错误关闭，并保留上一份认证目录。
        NetInterfaceFrame emptyDirectory = {};
        emptyDirectory.m_kind = NI_RESPONSE;
        emptyDirectory.m_messageType = NI_GATEWAY_DISCOVER;
        emptyDirectory.m_requestId = 2;
        CHECK_TEST(server.sendControl(1, emptyDirectory) == NI_OK);
        const uint64_t closeLimit = PeerOptionNow() + 3000;
        while (activeWire && PeerOptionNow() < closeLimit) { CHECK_TEST(advance()); }
        CHECK_TEST(!activeWire && closeReason == NI_PROTOCOL_ERROR && probe.discovered() == 1);
    }
    else
    {
        CHECK_TEST(listener.frames.size() == 1);
    }
    if (reply && !shortDeadline)
    {
        const uint8_t unexpected = 7;
        const NetInterfaceTunnelRecord record = {NI_TUNNEL_BYTES, 7, 1, &unexpected, 1};
        CHECK_TEST(server.sendRecord(1, record) == NI_OK);
        for (unsigned i = 0; i < 256 && activeWire; ++i) { CHECK_TEST(advance()); }
        CHECK_TEST(!activeWire && closeReason == NI_PROTOCOL_ERROR);
    }
    return 0;
}

/** 验证管理控制帧和透明记录逐项交付，前一项的背压时间不计入下一项
@param [in] node 监听端的隔离节点身份，借用至函数返回
@return 两条控制帧及一条透明记录各自解除背压后均只交付一次返回0
*/
static int CheckLinkBackpressureTurnover(const NetInterfaceLinkOptions& node)
{
    PeerTestListener receiving, sending;
    NetLinkTransportConfig config;
    config.m_backpressureMs = 25;
    NetLinkTransport server(config), client;
    CHECK_TEST(server.listen("127.0.0.1", 9500, node, &receiving) == NI_OK && activeWire);
    PeerTestWire& serverWire = *activeWire;
    NetInterfaceLinkOptions options = {};
    options.m_version = 3;
    options.m_role = NI_LINK_CLIENT;
    uint64_t peer = 0;
    CHECK_TEST(client.connect("127.0.0.1", 9500, options, &sending, peer) == NI_OK && activeWire && activeWire != &serverWire);
    PeerTestWire& clientWire = *activeWire;
    serverWire.capture = serverWire.live = clientWire.capture = true;
    serverWire.connected(serverWire.observer, peer);
    clientWire.connected(clientWire.observer, peer);
    // 推进真实握手并在两端之间交换已经输出的字节。
    const auto advance = [&]() -> bool {
        if (!client.poll() || !server.poll()) { return false; }
        TransferDiscoveryBytes(clientWire, serverWire);
        TransferDiscoveryBytes(serverWire, clientWire);
        return true;
    };
    const uint64_t handshakeEnd = PeerOptionNow() + 5000;
    while ((!receiving.readyCount || !sending.readyCount) && PeerOptionNow() < handshakeEnd) { CHECK_TEST(advance()); }
    CHECK_TEST(receiving.readyCount == 1 && sending.readyCount == 1);
    NetInterfaceFrame frame = {};
    frame.m_kind = NI_REQUEST;
    frame.m_messageType = NI_GATEWAY_DISCOVER;
    frame.m_requestId = 1;
    receiving.m_acceptControlThrough = 0;
    CHECK_TEST(client.sendControl(peer, frame) == NI_OK);
    for (unsigned i = 0; i < 256 && !receiving.m_refusedControl; ++i) { CHECK_TEST(advance()); }
    CHECK_TEST(receiving.m_refusedControl == 1 && receiving.frames.empty());
    // 只驱动发送端，令上一项的等待时间越过期限。把下一项的字节留给接收端的同一次 poll。
    const auto stageNext = [&]() -> bool {
        const uint64_t until = PeerOptionNow() + config.m_backpressureMs + 1;
        do
        {
            if (!client.poll()) { return false; }
        } while (PeerOptionNow() < until);
        if (clientWire.output.empty() || clientWire.output.size() > 1024 || serverWire.paused) { return false; }
        TransferDiscoveryBytes(clientWire, serverWire);
        return clientWire.output.empty();
    };
    frame.m_requestId = 2;
    CHECK_TEST(client.sendControl(peer, frame) == NI_OK && stageNext());
    receiving.m_acceptControlThrough = 1;
    CHECK_TEST(server.poll() && receiving.frames.size() == 1 && receiving.frames[0] == 1 &&
        receiving.m_refusedControl == 2 && serverWire.live);
    const uint8_t body[] = {0, 83};
    const NetInterfaceTunnelRecord record = {NI_TUNNEL_BYTES, 7, 1, body, sizeof(body)};
    CHECK_TEST(client.sendRecord(peer, record) == NI_OK && stageNext());
    receiving.m_acceptControlThrough = 2;
    receiving.m_rejectRelayRecord = true;
    CHECK_TEST(server.poll() && receiving.frames.size() == 2 && receiving.frames[1] == 2 &&
        receiving.m_relayOffers == 1 && receiving.m_relayBody.empty() && serverWire.live);
    receiving.m_rejectRelayRecord = false;
    CHECK_TEST(server.poll() && server.poll() && !receiving.closeCount && receiving.m_relayOffers == 2 &&
        receiving.m_relayBody.size() == sizeof(body) && !std::memcmp(receiving.m_relayBody.data(), body, sizeof(body)));
    return 0;
}

/** 加载本次测试网关创建的 NGI3 叶身份，在独立会话上检查发现与管理接收的时限。
@param [in] path 本次隔离网关的身份文件，仅在此调用期间使用
@return 各场景均通过时返回0，样本错误或任一检查失败返回1
*/
static int DiscoveryTimers(const char* path)
{
    CHECK_TEST(InstallWire() == NI_OK);
    std::unique_ptr<FILE, int (*)(FILE*)> file(std::fopen(path, "rb"), std::fclose);
    CHECK_TEST(file.get());
    const uint32_t capacity = 24577;
    // 释放缓冲前擦除整个分配范围，包含拒绝加载的样本。
    const auto release = [capacity](uint8_t* data) {
        NetInterface::wipe(data, capacity);
        delete[] data;
    };
    std::unique_ptr<uint8_t[], decltype(release)> storage(new uint8_t[capacity], release);
    uint8_t* const bytes = storage.get();
    const size_t size = std::fread(bytes, 1, capacity, file.get());
    CHECK_TEST(!std::ferror(file.get()) && size >= 48 && size < capacity && !std::memcmp(bytes, "NGI3", 4));
    uint32_t lengths[3] = {};
    for (uint32_t i = 0; i < 3; ++i)
    {
        for (uint32_t j = 0; j < 4; ++j)
        {
            lengths[i] = lengths[i] * 256 + bytes[4 + 4 * i + j];
        }
    }
    uint8_t digest[32];
    CHECK_TEST(lengths[0] == 37 && lengths[1] && lengths[1] <= 4096 && lengths[2] && lengths[2] <= 16384 &&
        48 + lengths[0] + lengths[1] + lengths[2] == size &&
        NetInterface::sha256(bytes, static_cast<uint32_t>(size - 32), digest, sizeof(digest)) == NI_OK &&
        !std::memcmp(bytes + size - 32, digest, sizeof(digest)));
    const std::string name(reinterpret_cast<const char*>(bytes + 16), lengths[0]);
    NetInterfaceLinkOptions node = {};
    node.m_version = 3;
    node.m_role = NI_LINK_NODE;
    node.m_name = name.c_str();
    node.m_peerRoles = 1U << NI_LINK_CLIENT;
    node.m_tls.server = 1;
    node.m_tls.privateKey = bytes + 16 + lengths[0];
    node.m_tls.privateKeySize = lengths[1];
    node.m_tls.certificate = node.m_tls.privateKey + lengths[1];
    node.m_tls.certificateSize = lengths[2];
    for (bool reply : {false, true})
    {
        for (bool shortDeadline : {false, true})
        {
            CHECK_TEST(CheckDiscoveryTimer(node, reply, shortDeadline) == 0);
        }
    }
    CHECK_TEST(CheckDiscoveryTimer(node, true, false, false, true) == 0);
    CHECK_TEST(CheckDiscoveryTimer(node, false, true, true) == 0);
    CHECK_TEST(CheckDiscoveryTimer(node, false, false, true) == 0);
    return CheckLinkBackpressureTurnover(node);
}

/** 核对同一候选的退避开关、抖动随机源和实例隔离
@return 全部通过为0
*/
static int DiscoveryBackoff()
{
    std::vector<NetGatewayAddress> seeds;
    CHECK_TEST(ParseTestSeeds("127.0.0.1:7010", seeds) == NI_OK);
    NetClientDiscoveryPoint point = {};
    point.m_structSize = sizeof(point);
    point.m_version = 1;
    unsigned entropyCalls = 0;
    NetInterfaceTlsOptions tls = {};
    tls.entropyContext = &entropyCalls;
    tls.entropy = [](void* context, uint8_t* out, uint32_t size) -> int32_t {
        ++*static_cast<unsigned*>(context);
        std::memset(out, 0, size);
        return 0;
    };
    NetGatewayDiscoveryConfig config;
    config.m_backoffMs = 0;
    config.m_jitterMs = 0;
    NetGatewayDiscovery immediate(config);
    CHECK_TEST(immediate.start(seeds, "", tls) == NI_OK && immediate.choose(point));
    immediate.failed(point);
    CHECK_TEST(entropyCalls == 0 && immediate.choose(point) && point.m_port == 7010);
    config.m_backoffMs = 60000;
    config.m_jitterMs = 100;
    NetGatewayDiscovery delayed(config);
    CHECK_TEST(delayed.start(seeds, "", tls) == NI_OK && delayed.choose(point));
    delayed.failed(point);
    CHECK_TEST(entropyCalls == 1 && !delayed.choose(point) && point.m_port == 0);
    CHECK_TEST(immediate.choose(point) && point.m_port == 7010);
    config.m_backoffMs = 0;
    config.m_jitterMs = 2;
    NetGatewayDiscovery retry(config);
    tls.entropy = [](void*, uint8_t*, uint32_t) -> int32_t { return -1; };
    CHECK_TEST(retry.start(seeds, "", tls) == NI_OK && retry.choose(point));
    retry.failed(point);
    // 配置允许的最长抖动仅1毫秒；一秒是真故障的退出保护。
    const auto deadline = PeerOptionNow() + UINT64_C(1000);
    while (!retry.choose(point) && PeerOptionNow() < deadline) {}
    CHECK_TEST(point.m_port == 7010);
    CHECK_TEST(InstallFileBackend() == 0 && InstallWire() == NI_OK);
    unsigned releases = 0;
    for (uint32_t retryMs : {2U, 60000U})
    {
        config.m_backoffMs = 0;
        config.m_jitterMs = 0;
        config.m_retryMs = retryMs;
        size_t attempts = 0;
        ObservePool(config.m_transport.m_peers, attempts);
        NetGatewayDiscovery reconnect(config);
        CHECK_TEST(reconnect.start(seeds) == NI_OK && reconnect.poll() && activeWire && attempts == 1);
        activeWire->live = false;
        activeWire->closed(activeWire->observer, 1, -83);
        const auto until = PeerOptionNow() + (retryMs == 2 ? 1000 : 8);
        do
        {
            CHECK_TEST(reconnect.poll());
        } while (attempts == 1 && PeerOptionNow() < until);
        CHECK_TEST(attempts == (retryMs == 2 ? 2U : 1U));
        reconnect.shutdown();
        CHECK_TEST(reconnect.stopped());
    }
    NetGatewayDiscovery cached;
    CHECK_TEST(cached.start(seeds, "peer.cache") == NI_OK && fileBackend);
    fileBackend->cache = true;
    fileBackend->m_releases = &releases;
    fileBackend->rejectNextRelease = true;
    CHECK_TEST(cached.poll() && fileBackend->completion.operation == NET_FILE_IO_OPEN_READ);
    ++fileBackend->completion.id;
    CHECK_TEST(cached.poll() && fileBackend->stopped && releases == 0);
    CHECK_TEST(cached.poll() && releases == 0);
    CHECK_TEST(cached.poll() && releases == 1 && cached.choose(point) && point.m_port == 7010);
    cached.shutdown();
    CHECK_TEST(cached.stopped() && releases == 1);
    return 0;
}

/** 种子轮选
@return 全部通过为0
*/
static int RepeatedOperations()
{
    std::vector<NetGatewayAddress> seeds;
    CHECK_TEST(ParseTestSeeds("127.0.0.1:7001,127.0.0.1:7002", seeds) == NI_OK && seeds.size() == 2);
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
            CHECK_TEST(endpoint.m_port == (i % 2 == 0 ? 7001U : 7002U));
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

/** 统一发送门面的生命周期、参数拒绝、票据和错误输出
@return 全部符合预期时返回0
*/
static int SenderFacade()
{
    CHECK_TEST(InstallWire() == NI_OK);
    NetSender& sender = NetSender::instance();
    CHECK_TEST(sender.pointerBytes() == sizeof(void*) && sender.configureWorkers(0) == NI_INVALID_ARGUMENT &&
        sender.configureWorkers(1) == NI_OK && sender.configureWorkers(2) == NI_NOT_SUPPORTED);
    uint8_t random[3] = {};
    CHECK_TEST(sender.randomBytes(random, 3) == NI_OK && random[0] == 7 && random[2] == 9);
    CHECK_TEST(&sender.backend() == &NetPeer::instance().backend() && &sender.fileProtocol() == &NetPeer::instance().fileProtocol());
    NetHostFunctions badHost = {};
    CHECK_TEST(sender.backend().configureHost(badHost) < 0);
    NetSenderClient* client = nullptr;
    CHECK_TEST(sender.createClient(client) == NI_OK && client);
    const auto release = [&](NetSenderClient* value) { sender.destroyClient(value); };
    std::unique_ptr<NetSenderClient, decltype(release)> owner(client, release);
    NetSenderGatewayOptions beforePrepare = {};
    CHECK_TEST(sender.connectServiceGateway(client, 0, beforePrepare, nullptr) == NI_INVALID_ARGUMENT);
    CHECK_TEST(sender.backend().configureGateways(client, "", nullptr) == NI_INVALID_STATE &&
        sender.lastGatewayError(client) == NI_INVALID_STATE);
    CHECK_TEST(sender.connectServiceGateway(client, 0, beforePrepare, nullptr) == NI_INVALID_ARGUMENT);
    CHECK_TEST(sender.configureCompression(client, nullptr, 0) == NI_INVALID_STATE &&
        sender.lastGatewayError(client) == NI_INVALID_STATE);
    CHECK_TEST(sender.connectServiceGateway(client, 0, beforePrepare, nullptr) == NI_INVALID_ARGUMENT);
    CHECK_TEST(sender.postRequest(client, 1, nullptr, 0) == NI_INVALID_STATE &&
        sender.lastGatewayError(client) == NI_INVALID_STATE);
    uint64_t ticket = 91, id = 91, generation = 91;
    bool ready = true;
    NetInterfaceFrame frame = {};
    frame.m_kind = NI_DATA; frame.m_productId = 7; frame.m_messageType = 11;
    uint64_t words[] = {31, 57, 83};
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(words);
    NetPeer& peer = NetPeer::instance();
    CHECK_TEST(peer.clientSendBegin(nullptr, 1, bytes + sizeof(words[0]) - 1, 2, &words[1]) == NI_INVALID_ARGUMENT && words[1] == 57);
    CHECK_TEST(sender.beginSendRequest(client, 1, bytes + 2 * sizeof(words[0]) - 1, 2, words[1]) == NI_INVALID_ARGUMENT && words[1] == 57);
    NetInterfaceFrame overlapFrame = frame;
    overlapFrame.m_requestId = 73;
    overlapFrame.m_payload = bytes + sizeof(words[0]);
    overlapFrame.m_payloadSize = 1;
    CHECK_TEST(peer.clientSendDataBegin(nullptr, &overlapFrame, &overlapFrame.m_requestId) == NI_INVALID_ARGUMENT && overlapFrame.m_requestId == 73);
    CHECK_TEST(sender.beginSendData(client, overlapFrame, words[1]) == NI_INVALID_ARGUMENT && words[1] == 57);
    CHECK_TEST(sender.beginSendData(client, overlapFrame, overlapFrame.m_requestId) == NI_INVALID_ARGUMENT && overlapFrame.m_requestId == 73);
    CHECK_TEST(peer.clientSendBegin(nullptr, 1, bytes, sizeof(words[0]), &words[1]) == NI_INVALID_ARGUMENT && words[1] == 0 && words[0] == 31);
    words[1] = 57;
    overlapFrame.m_payload = bytes + 2 * sizeof(words[0]);
    CHECK_TEST(sender.beginSendData(nullptr, overlapFrame, words[1]) == NI_INVALID_ARGUMENT && words[1] == 0 && words[2] == 83);
    CHECK_TEST(sender.beginSendRequest(nullptr, 1, nullptr, 0, ticket) == NI_INVALID_ARGUMENT && !ticket);
    ticket = 91;
    CHECK_TEST(sender.beginSendData(nullptr, frame, ticket) == NI_INVALID_ARGUMENT && !ticket);
    CHECK_TEST(sender.sendResult(nullptr, 1) == NI_INVALID_ARGUMENT && sender.postRequest(client, 1, nullptr, 0) == NI_INVALID_STATE);
    CHECK_TEST(sender.postData(client, frame) == NI_INVALID_STATE && sender.sendRequest(client, 1, nullptr, 0) == NI_INVALID_STATE &&
        sender.sendData(client, frame) == NI_INVALID_STATE && sender.cancelRequest(client, 1) == NI_INVALID_STATE);
    NetPeerConfig config;
    CHECK_TEST(NetSender::backend().prepareGateway(client, config) == NI_OK && sender.prepareGateway(client) == NI_OK);
    CHECK_TEST(sender.gatewayState(client, generation, ready) == NI_OK && !generation && !ready &&
        !sender.gatewayGeneration(client) && !sender.gatewayReady(client));
    NetConnectionPoolConfig connection;
    NetSenderSubmissionPools submission;
    LockFreeHashMapPoolConfig requests;
    CHECK_TEST(sender.configureConnectionPools(client, connection) == NI_OK && sender.configureRequestPool(client, requests) == NI_OK &&
        sender.configureSubmissionPools(client, submission) == NI_OK && sender.configureDispatch(client, true) == NI_OK);
    CHECK_TEST(sender.configureCompression(client, nullptr, 1) == NI_INVALID_ARGUMENT &&
        sender.lastGatewayError(client) == NI_INVALID_ARGUMENT && sender.configureCompression(client, nullptr, 0) == NI_OK);
    CHECK_TEST(sender.backend().configureGateways(client, "127.0.0.1:9180", nullptr) == NI_OK);
    NetSenderGatewayOptions invalid = {};
    CHECK_TEST(sender.connectGateway(client, invalid, nullptr) == NI_INVALID_ARGUMENT &&
        sender.connectServiceGateway(client, 0, invalid, nullptr) == NI_INVALID_ARGUMENT);
    CHECK_TEST(sender.beginRequest(client, 7, 11, 1000, id) < 0 && !id);
    ticket = 91;
    CHECK_TEST(sender.beginSendRequest(client, 1, nullptr, 0, ticket) < 0 && !ticket);
    ticket = 91;
    CHECK_TEST(sender.beginSendData(client, frame, ticket) < 0 && !ticket);
    CHECK_TEST(sender.sendResult(client, 73) == NI_NOT_FOUND);
    CHECK_TEST(sender.sendRequest(client, 73, nullptr, 0) < 0 && sender.sendData(client, frame) < 0 &&
        sender.cancelRequest(client, 73) < 0 && sender.postData(client, frame) < 0);
    sender.closeGateway(client, NI_CANCELLED);
    sender.uninitGateway(client);
    CHECK_TEST(sender.pollGateway(client) == NI_OK);
    owner.release();
    CHECK_TEST(sender.destroyClient(client) == NI_OK && !client && sender.destroyClient(client) == NI_OK);
    return 0;
}

/** 通过C入口完成真实TLS和业务握手，核对回调、背压、请求完成和关闭
@return 全部符合预期时返回0
*/
static int SenderC()
{
    CHECK_TEST(InstallWire() == NI_OK && netPeerPointerBytes() == sizeof(void*) &&
        netSenderConfigureWorkers(1) == NI_OK && netSenderClientDestroy(nullptr) == NI_OK);
    PeerSenderEvents events = {};
    events.valid = true;
    NetInterfaceClientCallbacks callbacks = {};
    callbacks.structSize = sizeof(callbacks);
    callbacks.version = 1;
    callbacks.context = &events;
    callbacks.ready = [](void* p, uint64_t g, uint32_t requests, uint32_t chunk) {
        PeerSenderEvents& e = *static_cast<PeerSenderEvents*>(p);
        e.generation = g;
        ++e.ready;
        e.valid = e.valid && g && requests == 4 && chunk == 4096 && netSenderClientPoll(e.handle) == NI_INVALID_STATE;
    };
    callbacks.result = [](void* p, uint64_t g, uint64_t id, uint32_t product, uint32_t type, uint32_t status,
        const uint8_t* bytes, uint32_t size) {
        PeerSenderEvents& e = *static_cast<PeerSenderEvents*>(p);
        ++e.results;
        e.valid = e.valid && g == e.generation && id == e.request && product == 7 && type == 11 &&
            status == NI_RPC_SUCCESS && size == 2 && bytes && bytes[0] == 0 && bytes[1] == 83;
    };
    callbacks.frame = [](void* p, uint64_t g, const NetInterfaceFrame* frame) -> int32_t {
        PeerSenderEvents& e = *static_cast<PeerSenderEvents*>(p);
        ++e.frames;
        e.valid = e.valid && g == e.generation && frame && frame->m_kind == NI_EVENT && frame->m_requestId == 77 &&
            frame->m_productId == 7 && frame->m_messageType == 11 && frame->m_payloadSize == 2 &&
            frame->m_payload && frame->m_payload[0] == 0 && frame->m_payload[1] == 83 &&
            netSenderClientDestroy(e.handle) == NI_INVALID_STATE;
        return e.frames > 1 ? 1 : 0;
    };
    callbacks.closed = [](void* p, uint64_t g, int32_t reason, int32_t) {
        PeerSenderEvents& e = *static_cast<PeerSenderEvents*>(p);
        ++e.closed;
        e.valid = e.valid && g == e.generation && reason == NI_CANCELLED;
    };
    CHECK_TEST(netSenderClientCreate(nullptr, &events.handle) == NI_INVALID_ARGUMENT && !events.handle &&
        netSenderClientCreate(&callbacks, &events.handle) == NI_OK && events.handle);
    /** 在失败退栈时关闭客户端，并有界推进到可销毁状态
    @param [in] h 当前测试拥有的非空句柄，清理成功后失效
    */
    const auto release = [](NetSenderClientHandle h)
    {
        const int32_t closing = netSenderClientClose(h, NI_CANCELLED);
        int32_t destroyed = NI_INVALID_STATE;
        int32_t polled = NI_OK;
        for (unsigned i = 0; i < 16 && destroyed == NI_INVALID_STATE; ++i)
        {
            polled = netSenderClientPoll(h);
            destroyed = netSenderClientDestroy(h);
        }
        // 正常路径显式销毁并release；此处只负责失败退栈，保留清理失败的具体诊断。
        if (closing != NI_OK || destroyed != NI_OK)
        {
            std::fprintf(stderr, "SenderC cleanup failed: close=%d poll=%d destroy=%d\n",
                static_cast<int>(closing), static_cast<int>(polled), static_cast<int>(destroyed));
        }
    };
    std::unique_ptr<NetPeerSenderClientOpaque, decltype(release)> owner(events.handle, release);
    CHECK_TEST(NetPeerTestCEntry(events.handle) == 1);
    uint64_t generation = 99, ticket = 99;
    int32_t ready = 7;
    CHECK_TEST(netSenderClientState(events.handle, &generation, &ready) == NI_OK && !ready && !generation &&
        netSenderClientConfigureDispatch(events.handle, 2) == NI_INVALID_ARGUMENT && netSenderClientConfigureDispatch(events.handle, 0) == NI_OK &&
        netSenderClientConfigureCompression(events.handle, nullptr, 0) == NI_OK && netSenderClientConfigureGateways(events.handle, nullptr, nullptr) == NI_OK &&
        netSenderClientSendDataBegin(events.handle, nullptr, &ticket) == NI_INVALID_ARGUMENT && !ticket);
    static const uint8_t ca[] = TEST_CA_CRT_RSA_SHA256_PEM, cert[] = TEST_SRV_CRT_RSA_SHA256_PEM, key[] = TEST_SRV_KEY_RSA_PEM;
    NetInterfaceClientOptions options = {};
    options.structSize = sizeof(options); options.version = 1;
    options.address = "127.0.0.1"; options.port = 9104; options.platform = 1;
    options.capabilities = 3; options.maxRequests = 4; options.maxChunk = 4096; options.handshakeTimeoutMs = 5000;
    options.tls.authority = ca; options.tls.authoritySize = sizeof(ca); options.tls.hostname = "localhost"; options.device[0] = 1;
    NetInterfaceTlsOptions tls = {};
    tls.server = 1; tls.certificate = cert; tls.certificateSize = sizeof(cert); tls.privateKey = key; tls.privateKeySize = sizeof(key);
    tls.entropy = [](void*, uint8_t* out, uint32_t size) -> int32_t { return NetPeer::randomBytes(out, size); };
    NetInterfaceConnection server(tls);
    NetInterfaceHandshakeOptions hello = {};
    hello.m_server = 1; hello.m_capabilities = 3; hello.m_maxRequests = 4; hello.m_maxChunk = 4096;
    hello.m_timeoutMs = 5000; hello.m_nonce[0] = 23;
    NetInterfaceHandshake handshake(hello, 0);
    CHECK_TEST(netSenderClientConnectService(events.handle, 0, 0, &options) == NI_INVALID_ARGUMENT &&
        netSenderClientConnect(events.handle, &options) == NI_OK && activeWire && activeWire->live);
    activeWire->capture = true;
    activeWire->connected(activeWire->observer, 1);
    const uint8_t body[] = {0, 83};
    unsigned requests = 0;
    const auto advance = [&]() -> bool {
        if (netSenderClientPoll(events.handle) < 0 || !activeWire || !activeWire->live || server.progress(1000) < 0) { return false; }
        std::vector<uint8_t>& wire = activeWire->output;
        if (!wire.empty())
        {
            uint32_t used = 0;
            if (server.feed(wire.data(), static_cast<uint32_t>(wire.size()), used) < 0 || used > wire.size()) { return false; }
            wire.erase(wire.begin(), wire.begin() + used);
        }
        NetInterfaceFrame input = {}, reply = {};
        if (server.peek(input) == NI_READY)
        {
            if (input.m_kind == NI_HELLO)
            {
                if (handshake.accept(input, 1) < 0 || handshake.peekOutput(reply) != NI_READY ||
                    server.send(reply) != NI_OK || handshake.consumeOutput() < 0) { return false; }
            }
            else
            {
                NetInterfaceRpcView rpc = {};
                uint8_t encoded[32] = {};
                uint32_t size = 0;
                if (input.m_kind != NI_REQUEST || input.m_productId != 7 || input.m_messageType != 11 ||
                    NetInterfaceRpc::parse(input, rpc) != NI_OK || rpc.m_size != sizeof(body) || !rpc.m_data ||
                    std::memcmp(rpc.m_data, body, sizeof(body)) || NetInterfaceRpc::encode(NI_RESPONSE, NI_RPC_SUCCESS, 0,
                    body, sizeof(body), encoded, sizeof(encoded), size) != NI_OK) { return false; }
                reply = input; reply.m_kind = NI_RESPONSE; reply.m_payload = encoded; reply.m_payloadSize = size;
                if (server.send(reply) != NI_OK) { return false; }
                ++requests;
            }
            if (server.consume() != NI_OK) { return false; }
        }
        const uint8_t* bytes = nullptr;
        uint32_t size = 0;
        if (server.peekOutput(bytes, size) < 0) { return false; }
        if (size && !activeWire->paused)
        {
            activeWire->data(activeWire->observer, 1, bytes, size);
            if (server.consumeOutput(size) != NI_OK) { return false; }
        }
        return true;
    };
    for (unsigned i = 0; i < 1024 && !events.ready; ++i) { CHECK_TEST(advance()); }
    CHECK_TEST(netPeerConfigureHost(nullptr) == NI_INVALID_ARGUMENT && netPeerRandomBytes(nullptr, 1) == NI_INVALID_ARGUMENT);
    CHECK_TEST(netSenderClientSendBegin(nullptr, 1, body, sizeof(body), &ticket) == NI_INVALID_ARGUMENT && !ticket &&
        netSenderClientSendResult(nullptr, 0) == NI_INVALID_ARGUMENT && netSenderClientPostData(nullptr, nullptr) == NI_INVALID_ARGUMENT &&
        netSenderClientSend(nullptr, 0, nullptr, 0) == NI_INVALID_ARGUMENT && netSenderClientSendData(nullptr, nullptr) == NI_INVALID_ARGUMENT &&
        netSenderClientCancel(nullptr, 0) == NI_INVALID_ARGUMENT);
    CHECK_TEST(events.ready == 1 && events.valid && netSenderClientBegin(events.handle, 7, 11, 5000, &events.request) == NI_OK && events.request);
    uint64_t words[] = {31, 57, 83};
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(words);
    CHECK_TEST(netSenderClientSendBegin(events.handle, events.request, bytes + sizeof(words[0]) - 1, 2, &words[1]) == NI_INVALID_ARGUMENT &&
        words[0] == 31 && words[1] == 57);
    NetInterfaceFrame data = {7, 73, 11, NI_DATA, 2, bytes + 2 * sizeof(words[0]) - 1};
    CHECK_TEST(netSenderClientSendDataBegin(events.handle, &data, &words[1]) == NI_INVALID_ARGUMENT && words[1] == 57 && words[2] == 83);
    CHECK_TEST(netSenderClientSendDataBegin(events.handle, &data, &data.m_requestId) == NI_INVALID_ARGUMENT && data.m_requestId == 73);
    CHECK_TEST(netSenderClientPost(events.handle, events.request, body, sizeof(body)) == NI_OK);
    for (unsigned i = 0; i < 1024 && !events.results; ++i) { CHECK_TEST(advance()); }
    CHECK_TEST(requests == 1 && events.results == 1 && events.valid);
    const NetInterfaceFrame event = {7, 77, 11, NI_EVENT, sizeof(body), body};
    CHECK_TEST(server.send(event) == NI_OK);
    for (unsigned i = 0; i < 1024 && events.frames < 2; ++i) { CHECK_TEST(advance()); }
    CHECK_TEST(events.frames == 2 && events.valid && netSenderClientClose(events.handle, NI_CANCELLED) == NI_OK);
    for (unsigned i = 0; i < 16 && !events.closed; ++i) { CHECK_TEST(netSenderClientPoll(events.handle) >= 0); }
    CHECK_TEST(events.closed == 1 && events.valid && netSenderClientDestroy(events.handle) == NI_OK);
    owner.release();
    return 0;
}

/** 检查默认与独立运输适配器、宿主异常、端点计数与提交端口借用
@return 全部符合预期时返回0
*/
static int TransportAdapters()
{
    CHECK_TEST(InstallWire() == NI_OK);
    PeerTestListener listener;
    NetHostSocket socket;
    uint64_t id = 29;
    CHECK_TEST(!socket.peerCount() && socket.connect(nullptr, 1234, &listener, id) == NI_INVALID_ARGUMENT && !id);
    failConnect = true;
    CHECK_TEST(socket.connect("127.0.0.1", 9124, &listener, id) == NI_INVALID_STATE && !id &&
        socket.status() == NI_INVALID_STATE && !socket.peerCount());
    NetHostSocket healthy;
    CHECK_TEST(healthy.connect("127.0.0.1", 9124, &listener, id) == NI_OK && id && healthy.peerCount() == 1);
    healthy.close(id);
    healthy.poll();
    CHECK_TEST(healthy.peerCount() == 0);
    NetGatewayNativeTransport transport;
    NetLinkTransport link;
    const NetInterfaceFrame frame = {};
    const NetInterfaceTunnelRecord record = {};
    CHECK_TEST(!link.localPort() && link.outputState(1) == NI_INVALID_STATE && link.sendRecord(1, record) == NI_INVALID_STATE &&
        transport.outputState(1) == NI_INVALID_STATE && transport.sendRecord(1, record) == NI_INVALID_STATE &&
        transport.sendControl(1, frame) == NI_INVALID_STATE);
    NetPeerEndpointRouter first(nullptr);
    NetDirectTransportConfig config;
    NetPeerEndpointRouter second(nullptr, nullptr, config);
    const NetInterfaceTlsOptions tls = {};
    CHECK_TEST(first.listen(nullptr, 0, tls) == NI_INVALID_ARGUMENT && second.listen("127.0.0.1", 65536, tls) == NI_INVALID_ARGUMENT &&
        !first.localPort() && !second.localPort());
    std::vector<NetInterfaceDirectoryPoint> candidates(1);
    candidates[0].m_port = 91;
    CHECK_TEST(NetGatewayDirectoryStore::load("", candidates) == NI_NOT_SUPPORTED && candidates.size() == 1 && candidates[0].m_port == 91);
    NetPeerClientSession session;
    CHECK_TEST(!session.ready() && !session.generation() && session.bindChannel(nullptr) == NI_OK);
    NetPeerClientCallbacks callbacks = {};
    callbacks.structSize = sizeof(callbacks); callbacks.version = 1;
    callbacks.ready = [](void*, uint64_t, uint32_t, uint32_t) {};
    callbacks.result = [](void*, uint64_t, uint64_t, uint32_t, uint32_t, uint32_t, const uint8_t*, uint32_t) {};
    callbacks.frame = [](void*, uint64_t, const NetInterfaceFrame*) -> int32_t { return 1; };
    callbacks.closed = [](void*, uint64_t, int32_t, int32_t) {};
    NetSenderSubmission submission;
    NetPeerClientHandle handle = nullptr;
    NetPeer& api = NetPeer::instance();
    CHECK_TEST(api.clientCreate(&callbacks, &handle) == NI_OK && handle);
    const auto release = [&api](NetPeerClientHandle h) { api.clientDestroy(h); };
    std::unique_ptr<NetPeerClientOpaque, decltype(release)> owner(handle, release);
    NetSenderSubmissionPools pools;
    CHECK_TEST(api.clientConfigureSubmissionPools(handle, pools) == NI_OK &&
        NetPeerClientBackend::bindSubmission(nullptr, &submission) == NI_INVALID_ARGUMENT &&
        NetPeerClientBackend::bindSubmission(handle, nullptr) == NI_INVALID_ARGUMENT &&
        NetPeerClientBackend::bindSubmission(handle, &submission) == NI_OK);
    return 0;
}

/** 经公开 C 文件门面检查重叠时完全不写、普通失败时写入约定值
@param [in] live 有效文件流，仅作为独立输出的初始值
@return 全部输出符合预期时返回0
*/
static int CheckFileCOutputs(NetInterfaceFileFlowHandle live)
{
    const auto filled = [](const void* data, size_t size, uint8_t value) -> bool {
        const uint8_t* first = static_cast<const uint8_t*>(data);
        return std::all_of(first, first + size, [value](uint8_t byte) { return byte == value; });
    };
    NetInterfaceFileInfo info;
    std::memset(&info, 0x53, sizeof(info));
    const uint8_t* input = reinterpret_cast<const uint8_t*>(&info);
    CHECK_TEST(netPeerFileParseInfo(input, sizeof(info), &info) == NI_INVALID_ARGUMENT && filled(&info, sizeof(info), 0x53));
    CHECK_TEST(netPeerFileFlowCreate(&info, 7, 1, 1, 0, 1000, reinterpret_cast<NetInterfaceFileFlowHandle*>(&info)) ==
        NI_INVALID_ARGUMENT && filled(&info, sizeof(info), 0x53));
    CHECK_TEST(netPeerFileFlowSnapshot(nullptr, &info, reinterpret_cast<int32_t*>(&info)) == NI_INVALID_ARGUMENT &&
        filled(&info, sizeof(info), 0x53));
    CHECK_TEST(netPeerFileParseInfo(nullptr, 0, &info) == NI_PROTOCOL_ERROR && filled(&info, sizeof(info), 0));

    uint32_t fields[12];
    uint8_t transfer[16], token[32], hash[32];
    std::memset(fields, 0x53, sizeof(fields));
    std::memset(transfer, 0x53, sizeof(transfer));
    std::memset(token, 0x53, sizeof(token));
    std::memset(hash, 0x53, sizeof(hash));
    CHECK_TEST(netPeerFileInfoFields(nullptr, 0, fields, token + 1, token, hash) == NI_INVALID_ARGUMENT &&
        filled(fields, sizeof(fields), 0x53) && filled(token, sizeof(token), 0x53) && filled(hash, sizeof(hash), 0x53));
    CHECK_TEST(netPeerFileControlEncode(fields, transfer, token, hash, nullptr, 0, nullptr, 0, nullptr, 0, fields + 1) ==
        NI_INVALID_ARGUMENT && filled(fields, sizeof(fields), 0x53));
    CHECK_TEST(netPeerFileInfoFields(nullptr, 0, fields, transfer, token, hash) == NI_PROTOCOL_ERROR &&
        filled(fields, sizeof(fields), 0) && filled(transfer, sizeof(transfer), 0) && filled(token, sizeof(token), 0) && filled(hash, sizeof(hash), 0));
    uint32_t required = 83;
    CHECK_TEST(netPeerFileControlEncode(nullptr, transfer, token, hash, nullptr, 0, nullptr, 0, nullptr, 0, &required) ==
        NI_INVALID_ARGUMENT && required == 0);

    NetInterfaceFileFlowHandle output = live;
    CHECK_TEST(netPeerFileFlowCreateWire(reinterpret_cast<const uint8_t*>(&output), sizeof(output), 7, 1, 1, 0, 1000, &output) ==
        NI_INVALID_ARGUMENT && output == live);
    CHECK_TEST(netPeerFileFlowCreateWire(nullptr, 0, 7, 1, 1, 0, 1000, &output) == NI_PROTOCOL_ERROR && output == nullptr);
    output = live;
    CHECK_TEST(netPeerFileFlowCreate(nullptr, 7, 1, 1, 0, 1000, &output) == NI_INVALID_ARGUMENT && output == nullptr);

    union { const uint8_t* pointer; uint32_t fields[6]; } alias;
    std::memset(&alias, 0x53, sizeof(alias));
    CHECK_TEST(netPeerFileFlowWrite(nullptr, alias.fields, &alias.pointer) == NI_INVALID_ARGUMENT && filled(&alias, sizeof(alias), 0x53));
    CHECK_TEST(netPeerFileFlowPeek(nullptr, alias.fields, &alias.pointer) == NI_INVALID_ARGUMENT && filled(&alias, sizeof(alias), 0x53));
    const uint8_t* view = token;
    std::memset(fields, 0x53, sizeof(fields));
    CHECK_TEST(netPeerFileFlowWrite(nullptr, fields, &view) == NI_INVALID_ARGUMENT && !view && filled(fields, 3 * sizeof(*fields), 0));
    view = token;
    std::memset(fields, 0x53, sizeof(fields));
    CHECK_TEST(netPeerFileFlowPeek(nullptr, fields, &view) == NI_INVALID_ARGUMENT && !view && filled(fields, 6 * sizeof(*fields), 0));

    int32_t status = 83;
    std::memset(&info, 0x53, sizeof(info));
    CHECK_TEST(netPeerFileFlowSnapshot(nullptr, &info, &status) == NI_INVALID_ARGUMENT && status == NI_INVALID_ARGUMENT &&
        filled(&info, sizeof(info), 0));
    status = 83;
    CHECK_TEST(netPeerFileFlowSnapshotWire(live, reinterpret_cast<uint8_t*>(&status), sizeof(status), &status) ==
        NI_INVALID_ARGUMENT && status == 83);
    CHECK_TEST(netPeerFileFlowSnapshotWire(live, nullptr, 0, &status) == NI_BUFFER_TOO_SMALL && status == NI_INVALID_STATE);
    return 0;
}

/** 通过C文件接口传输包含零字节的文件，逐次核对落盘、确认、提交和最终快照
@return 全部符合预期时返回0
*/
static int FileC()
{
    const uint8_t body[] = {0, 31, 83};
    NetInterfaceFileInfo info = {};
    info.direction = 1; info.state = NI_FILE_TRANSFERRING; info.chunk = 1024; info.window = 1;
    info.total = sizeof(body); info.stream = 43; info.expiresUtc = 1000; info.transfer[0] = 7; info.token[0] = 11;
    CHECK_TEST(NetInterface::sha256(body, sizeof(body), info.hash, sizeof(info.hash)) == NI_OK);
    uint8_t bytes[NET_INTERFACE_FILE_INFO_SIZE] = {};
    CHECK_TEST(NetInterfaceFile::encodeInfo(info, bytes, sizeof(bytes)) == NI_OK);
    NetInterfaceFileInfo decoded = {};
    uint32_t fields[12] = {};
    uint8_t transfer[16], token[32], hash[32];
    CHECK_TEST(netPeerFileParseInfo(bytes, sizeof(bytes), &decoded) == NI_OK && decoded.stream == 43 &&
        netPeerFileInfoFields(bytes, sizeof(bytes), fields, transfer, token, hash) == NI_OK &&
        fields[4] == sizeof(body) && transfer[0] == 7 && token[0] == 11 && !std::memcmp(hash, info.hash, sizeof(hash)));
    const uint32_t control[] = {NI_FILE_OPEN_UPLOAD, 1024, 1, sizeof(body), 0};
    uint8_t emptyToken[32] = {};
    uint32_t required = 0;
    CHECK_TEST(netPeerFileControlEncode(control, transfer, emptyToken, hash, body, sizeof(body), nullptr, 0, nullptr, 0, &required) ==
        NI_BUFFER_TOO_SMALL && required == NET_INTERFACE_FILE_CONTROL_HEADER_SIZE + sizeof(body));
    NetInterfaceFileFlowHandle source = nullptr, target = nullptr;
    CHECK_TEST(netPeerFileFlowCreate(&info, 7, 1, 1, 0, 1000, &source) == NI_OK && source);
    std::unique_ptr<NetInterfaceFileFlowOpaque, void (*)(NetInterfaceFileFlowHandle)> left(source, netPeerFileFlowDestroy);
    CHECK_TEST(CheckFileCOutputs(source) == 0);
    CHECK_TEST(netPeerFileFlowCreateWire(bytes, sizeof(bytes), 7, 0, 1, 0, 1000, &target) == NI_OK && target);
    std::unique_ptr<NetInterfaceFileFlowOpaque, void (*)(NetInterfaceFileFlowHandle)> right(target, netPeerFileFlowDestroy);
    CHECK_TEST(netPeerFileFlowSetWriteBatchChunks(target, 1) == NI_OK && netPeerFileFlowPrime(source, 0, 0, body, 1) == NI_INVALID_ARGUMENT &&
        netPeerFileFlowProgress(source, 2, 0) == NI_OK && netPeerFileFlowRead(source, fields) == NI_READY &&
        fields[0] == 0 && fields[1] == 0 && fields[2] == sizeof(body) && netPeerFileFlowProduced(source, 0, 0, body, sizeof(body)) == NI_OK);
    const auto deliver = [](NetInterfaceFileFlowHandle from, NetInterfaceFileFlowHandle to, uint32_t kind) -> bool {
        uint32_t frame[6] = {};
        const uint8_t* data = nullptr;
        return netPeerFileFlowPeek(from, frame, &data) == NI_READY && frame[0] == kind && frame[1] == 7 && frame[2] == 43 &&
            frame[5] && data && netPeerFileFlowAccept(to, frame, data) == NI_OK && netPeerFileFlowConsume(from) == NI_OK;
    };
    const uint8_t* view = nullptr;
    CHECK_TEST(deliver(source, target, NI_DATA) && netPeerFileFlowWrite(target, fields, &view) == NI_READY && view &&
        !fields[0] && !fields[1] && fields[2] == sizeof(body) && !std::memcmp(view, body, sizeof(body)));
    CHECK_TEST(netPeerFileFlowWritten(target, 0, 0, sizeof(body), NI_OK) == NI_OK && deliver(target, source, NI_DATA_ACK) &&
        deliver(source, target, NI_DATA) && netPeerFileFlowTakeCommit(target) == NI_READY &&
        netPeerFileFlowTakeCommit(target) == NI_NEED_MORE && netPeerFileFlowCommitted(target, NI_OK) == NI_READY &&
        deliver(target, source, NI_DATA_ACK));
    int32_t status = NI_OK;
    CHECK_TEST(netPeerFileFlowSnapshot(target, &decoded, &status) == NI_OK && status == NI_READY &&
        decoded.state == NI_FILE_COMPLETED && !decoded.stream && decoded.offset == sizeof(body));
    CHECK_TEST(netPeerFileFlowSnapshotWire(source, bytes, sizeof(bytes), &status) == NI_OK && status == NI_READY &&
        netPeerFileParseInfo(bytes, sizeof(bytes), &decoded) == NI_OK && decoded.state == NI_FILE_COMPLETED && !decoded.stream);
    CHECK_TEST(netPeerFileFlowCancel(nullptr, NI_CANCELLED) == NI_INVALID_ARGUMENT);
    NetInterfaceFileFlowHandle failedSender = nullptr;
    CHECK_TEST(netPeerFileFlowCreate(&info, 7, 1, 1, 0, 1000, &failedSender) == NI_OK && failedSender);
    std::unique_ptr<NetInterfaceFileFlowOpaque, void (*)(NetInterfaceFileFlowHandle)> failed(failedSender, netPeerFileFlowDestroy);
    NetInterfaceFileAck failure = {};
    failure.flags = 2;
    failure.result = NI_LIMIT_EXCEEDED;
    std::memcpy(failure.token, info.token, sizeof(info.token));
    uint8_t ack[NET_INTERFACE_FILE_ACK_SIZE] = {};
    const uint32_t ackFields[] = {NI_DATA_ACK, 7, 43, 0, NI_FILE_DATA_TYPE, sizeof(ack)};
    CHECK_TEST(NetInterfaceFile::encodeAck(failure, ack, sizeof(ack)) == NI_OK &&
        netPeerFileFlowAccept(failedSender, ackFields, ack) == NI_LIMIT_EXCEEDED &&
        netPeerFileFlowProgress(failedSender, 2, 0) == NI_LIMIT_EXCEEDED);
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
    if (argc == 4 && std::strcmp(argv[1], "--service-options") == 0)
    {
        return ServiceOptions(argv[2], argv[3]);
    }
    if (argc == 3 && std::strcmp(argv[1], "--discovery-timers") == 0)
    {
        return DiscoveryTimers(argv[2]);
    }
    if (argc == 2 && std::strcmp(argv[1], "--benchmark") == 0)
    {
        return Benchmark();
    }
    const LibraryTestCase cases[] = {
        {NetPeerTestLifecycle, false, Lifecycle},
        {NetPeerTestConfiguration, false, Configuration},
        {NetPeerTestDiscovery, false, Discovery},
        {NetPeerTestRepeatedOperations, true, RepeatedOperations},
        {NetPeerTestPreparedConfiguration, false, PreparedConfiguration},
        {NetPeerTestDirectTransport, false, DirectTransport},
        {NetPeerTestLinkBudgets, false, LinkBudgets},
        {NetPeerTestEntropyBounds, false, EntropyBounds},
        {NetPeerTestHostInput, false, HostInput},
        {NetPeerTestFileBatch, false, FileBatch},
        {NetPeerTestNumericOptions, false, NumericOptions},
        {NetPeerTestClientPaths, false, ClientPaths},
        {NetPeerTestTlsFrames, false, TlsFrames},
        {NetPeerTestRelayPools, false, RelayPools},
        {NetPeerTestDiscoveryBackoff, false, DiscoveryBackoff},
        {NetPeerTestSenderFileClose, false, SenderFileClose},
        {NetPeerTestCacheWrites, false, CacheWrites},
        {NetPeerTestCacheRoundTrip, false, CacheRoundTrip},
        {NetPeerTestWriteLength, false, WriteLength},
        {NetPeerTestTlsQueueLimits, false, TlsQueueLimits},
        {NetPeerTestEndpointRetry, false, EndpointRetry},
        {NetPeerTestProgressTimeout, false, FileProgressTimeout},
        {NetPeerTestEarlyEof, false, EarlyEof},
        {NetPeerTestLargeTlsWrite, false, LargeTlsWrite},
        {NetPeerTestSenderFacade, false, SenderFacade},
        {NetPeerTestSenderC, false, SenderC},
        {NetPeerTestTransportAdapters, false, TransportAdapters},
        {NetPeerTestFileC, false, FileC}
    };
    return LibraryCaseSelection::run(argc, argv, cases);
}
catch (const std::exception& error)
{
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}