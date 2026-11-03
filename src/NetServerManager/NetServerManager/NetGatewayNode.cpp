#include "NetGatewayNode.h"
#include "NetInterface/NetInterfaceAPI.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include <algorithm>
#include <set>
#include <chrono>
#include <cstring>
#include <cstdio>
#include "NetGatewayAuthority.h"
#include "NetServerEnrollment.h"
#include <sstream>
#include <vector>

static const uint64_t g_meshLocal = UINT64_C(0x8000000000000000);

static const uint64_t g_meshOutbound = UINT64_C(0x4000000000000000);

/** 节点发现、快照及连接期限使用同一单调时间
@return 单调毫秒
*/
static uint64_t MeshNow()
{
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

/** 精确读取管理整数，不接受字符串/浮点隐式转换
@param [in] message protobuf
@param [in] field 全局键
@param [out] output 非负数值
@return 字段存在且为合法整数
*/
static bool GatewayNodeNumber(const ProtoMessage& message, int32_t field, uint64_t& output)
{
    Variant value;
    if (!message.getKeyValue(field, value)) { return false; }
    if (value.type() == Variant::UINT64) { output = static_cast<uint64_t>(value); return true; }
    if (value.type() != Variant::INT32 || static_cast<int32_t>(value) < 0) { return false; }
    output = static_cast<uint32_t>(static_cast<int32_t>(value));
    return true;
}

/** 节点/服务身份仅使用系统强随机
@param [in] context 忽略
@param [out] output 输出
@param [in] size 字节数
@return 0成功，否则失败
*/
static int32_t GatewayNodeEntropy(void* context, uint8_t* output, uint32_t size)
{
    (void)context;
    return NetTransport::randomBytes(output, size) == NI_OK ? 0 : -1;
}

/** 仅本机管理监听安装；先验证服务凭证，随后才允许CA签发精确CSR身份
@param [in] context 忽略
@param [in] peer 管理层已验证的规范服务名/CSR
@param [in] statement 含双方新挑战的完整声明
@param [in] statementSize 声明长度
@param [in] proof 注册HMAC
@param [in] proofSize 必须32字节
@param [out] grant 公开叶证书
@param [in] capacity 输出容量
@param [out] grantSize 成功字节数
@return NI_OK或统一拒绝错误
*/
static int32_t GatewayNodeEnroll(void* context, const NetInterfaceLinkPeer* peer, const uint8_t* statement,
    uint32_t statementSize, const uint8_t* proof, uint32_t proofSize, uint8_t* grant, uint32_t capacity, uint32_t* grantSize)
{
    (void)context;
    if (grantSize == nullptr) { return NI_INVALID_ARGUMENT; }
    *grantSize = 0;
    if (peer == nullptr || peer->m_role != NI_LINK_SERVICE || peer->m_name == nullptr || peer->m_claims == nullptr ||
        peer->m_claimsSize == 0 || peer->m_claimsSize > 4096 || grant == nullptr ||
        NetServerEnrollment::verify(statement, statementSize, proof, proofSize) != NI_OK)
    {
        return NI_PROTOCOL_ERROR;
    }
    std::string certificate;
    const std::string request(reinterpret_cast<const char*>(peer->m_claims), peer->m_claimsSize);
    if (NetGatewayAuthority::issueLocalIdentity(peer->m_name, request, GatewayNodeEntropy, nullptr, certificate) != NI_OK ||
        certificate.size() > capacity)
    {
        return NI_PROTOCOL_ERROR;
    }
    std::memcpy(grant, certificate.data(), certificate.size());
    *grantSize = static_cast<uint32_t>(certificate.size());
    return NI_OK;
}

/** 网关路由和控制队列使用单调期限
@return 单调毫秒
*/
static uint64_t GatewayNodeNow()
{
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

// 原适配的代次严格递增；最高位仅为本进程内IPC/TCP作用域，不来自网络输入。
static const uint64_t g_nodeLocalPeer = UINT64_C(0x8000000000000000);
static const uint64_t g_nodeOutboundPeer = UINT64_C(0x4000000000000000);

NetGatewayNode::NetGatewayNode(const std::string& name, const std::string& certificate, const std::string& privateKey) :
    m_localListener(*this), m_local(m_inbox), m_name(name), m_certificate(certificate), m_privateKey(privateKey),
    m_boot(0), m_snapshotRevision(0), m_snapshotAt(0), m_lastOutbound(0), m_meshDirty(true),
    m_capacity(0), m_lastRoute(0), m_recordBytes(0), m_receiving(false), m_stopping(false)
{
    m_cacheEnabled = false;
    m_cacheDirty = true;
    m_cacheQueued = 0;
}

NetGatewayNode::~NetGatewayNode()
{
    shutdown();
    if (!m_privateKey.empty())
    {
        NetInterface::wipe(reinterpret_cast<uint8_t*>(&m_privateKey[0]), static_cast<uint32_t>(m_privateKey.size()));
    }
}

int32_t NetGatewayNode::listen(const char* address, uint32_t port, const std::string& advertise, uint32_t capacity)
{
    if (m_stopping || m_receiving || !NetInterfaceDirectory::address(advertise) || capacity == 0 || capacity > 256 ||
        m_name.size() != 37 || m_name.compare(0, 5, "node.") != 0 ||
        m_name.find_first_not_of("0123456789abcdef", 5) != std::string::npos)
    {
        return NI_INVALID_ARGUMENT;
    }
    m_advertise = advertise;
    m_capacity = capacity;
    if (GatewayNodeEntropy(nullptr, reinterpret_cast<uint8_t*>(&m_boot), sizeof(m_boot)) != 0 || m_boot == 0) { return NI_INVALID_STATE; }
    NetInterfaceLinkOptions options = {};
    options.m_version = 3;
    options.m_role = NI_LINK_NODE;
    options.m_name = m_name.c_str();
    options.m_peerRoles = 1U << NI_LINK_SERVICE;
    options.m_serviceVerify = GatewayNodeEnroll;
    options.m_tls.server = 1;
    options.m_tls.certificate = reinterpret_cast<const uint8_t*>(m_certificate.c_str());
    options.m_tls.certificateSize = static_cast<uint32_t>(m_certificate.size() + 1);
    options.m_tls.privateKey = reinterpret_cast<const uint8_t*>(m_privateKey.c_str());
    options.m_tls.privateKeySize = static_cast<uint32_t>(m_privateKey.size() + 1);
    options.m_tls.handshakeTimeoutMs = 5000;
    if (ProcessWork::instance().receiveBufferCount() != 0)
    {
        return NI_INVALID_STATE;
    }
    ProcessWork::instance().addProcessReceiveCallback(&m_inbox);
    if (!ProcessWork::instance().initReceive(16452, 64))
    {
        ProcessWork::instance().uninitReceive();
        m_local.shutdown();
        return NI_INVALID_STATE;
    }
    m_receiving = true;
    // 先确认接收声明，再开放服务注册；后续错误均回收本节点的接收器。
    int32_t started = m_local.listen(options, &m_localListener, 64);
    if (started != NI_OK)
    {
        shutdown(); return started;
    }
    options.m_peerRoles = (1U << NI_LINK_CLIENT) | (1U << NI_LINK_NODE);
    options.m_serviceVerify = nullptr;
    started = m_transport.listen(address, port, options, this, capacity);
    if (started != NI_OK) { shutdown(); }
    return started;
}

bool NetGatewayNode::poll()
{
    if (m_stopping)
    {
        m_cache.poll();
        return !m_cache.stopped();
    }
    if (!m_receiving)
    {
        return false;
    }
    m_local.poll();
    const bool active = m_transport.poll();
    pollMesh();
    flushControls();
    flushRecords();
    return active && !m_stopping;
}

void NetGatewayNode::shutdown()
{
    if (m_stopping) { return; }
    m_stopping = true;
    m_cache.shutdown();
    m_local.shutdown();
    m_transport.shutdown();
    for (auto& connection : m_outbound) { connection.second->shutdown(); }
    m_outbound.clear();
    m_routes.clear();
    m_recordBytes = 0;
    m_channels.clear();
    m_peers.clear();
    m_pendingOpens.clear();
    if (m_receiving) { ProcessWork::instance().uninitReceive(); m_receiving = false; }
}

uint32_t NetGatewayNode::localPort() const
{
    return m_transport.localPort();
}

void NetGatewayNode::onLinkReady(uint64_t peer, const NetInterfaceLinkPeer& identity)
{
    const bool local = (peer & g_nodeLocalPeer) != 0;
    if (m_stopping || peer == 0 || (!local && peer >= g_nodeLocalPeer) || identity.m_name == nullptr ||
        (local ? identity.m_role != NI_LINK_SERVICE : identity.m_role == NI_LINK_SERVICE))
    {
        close(peer, NI_PROTOCOL_ERROR);
        return;
    }
    if (identity.m_role == NI_LINK_NODE && (m_name == identity.m_name || std::strlen(identity.m_name) != 37 ||
        std::string(identity.m_name).compare(0, 5, "node.") != 0 ||
        std::string(identity.m_name).find_first_not_of("0123456789abcdef", 5) != std::string::npos))
    {
        close(peer, NI_PROTOCOL_ERROR);
        return;
    }
    NetGatewayNodePeer entry;
    entry.m_role = identity.m_role;
    entry.m_name = identity.m_name;
    if (local)
    {
        // Link核心已校验service.<规范uint32>，这里仅转换，不接受另一个产品声明。
        uint64_t product = 0;
        for (size_t index = 8; index < entry.m_name.size(); ++index) { product = product * 10 + entry.m_name[index] - '0'; }
        if (product > UINT32_MAX) { close(peer, NI_PROTOCOL_ERROR); return; }
        entry.m_product = static_cast<uint32_t>(product);
    }
    if (!m_peers.insert(std::make_pair(peer, entry)).second)
    {
        close(peer, NI_PROTOCOL_ERROR);
    }
}

bool NetGatewayNode::controlSpace(uint64_t peer, uint32_t bytes) const
{
    auto found = m_peers.find(peer);
    return found != m_peers.end() && !found->second.m_closing && bytes <= 16384 &&
        found->second.m_controls.size() < 32 && found->second.m_controlBytes <= 65536 - bytes;
}

int32_t NetGatewayNode::queueControl(uint64_t peer, const NetInterfaceFrame& frame, const std::string& bytes)
{
    auto target = m_peers.find(peer);
    if (target == m_peers.end() || target->second.m_closing) { return NI_LIMIT_EXCEEDED; }
    NetGatewayNodePeer& destination = target->second;
    const bool closing = frame.m_kind == NI_EVENT && frame.m_messageType == NI_GATEWAY_CHANNEL_CLOSE &&
        frame.m_requestId == 0 && frame.m_productId == 0;
    if (closing)
    {
        // 节点至多4096条活跃路由，关闭各有一次通知；与32项普通预算分开，避免批量退出误杀共享服务。
        // 普通接纳仍受原controlSpace限制，通知积压时不继续OPEN；每轮只发送原有32步，五秒停滞仍收敛。
        if (bytes.size() > 128 || destination.m_controls.size() >= 32 + 4096 ||
            destination.m_controlBytes > 65536 + 4096 * 128 - bytes.size()) { return NI_LIMIT_EXCEEDED; }
    }
    else if (!controlSpace(peer, static_cast<uint32_t>(bytes.size()))) { return NI_LIMIT_EXCEEDED; }
    destination.m_controls.push_back(NetGatewayNodeControl(frame, bytes));
    destination.m_controlBytes += static_cast<uint32_t>(bytes.size());
    return NI_OK;
}

void NetGatewayNode::flushControls()
{
    for (auto& item : m_peers)
    {
        NetGatewayNodePeer& peer = item.second;
        for (uint32_t step = 0; step < 32 && !peer.m_closing && !peer.m_controls.empty(); ++step)
        {
            NetGatewayNodeControl& pending = peer.m_controls.front();
            NetInterfaceFrame frame = pending.m_frame;
            frame.m_payload = reinterpret_cast<const uint8_t*>(pending.m_payload.data());
            const int32_t sent = sendControl(item.first, frame);
            if (sent == NI_LIMIT_EXCEEDED)
            {
                const uint64_t now = GatewayNodeNow();
                if (peer.m_blockedSince == 0) { peer.m_blockedSince = now; }
                else if (now - peer.m_blockedSince >= 5000) { close(item.first, NI_TIMEOUT); }
                break;
            }
            if (sent != NI_OK) { close(item.first, sent); break; }
            peer.m_blockedSince = 0;
            peer.m_controlBytes -= static_cast<uint32_t>(pending.m_payload.size());
            peer.m_controls.pop_front();
        }
    }
}

int32_t NetGatewayNode::reply(uint64_t peer, const NetInterfaceFrame& original, int32_t status, const std::string& bytes)
{
    ProtoMessage response;
    if (!bytes.empty() && !response.from(bytes)) { return NI_PROTOCOL_ERROR; }
    response.setCode(status);
    std::string encoded;
    if (!response.toString(encoded)) { return NI_PROTOCOL_ERROR; }
    NetInterfaceFrame frame = original;
    frame.m_kind = NI_RESPONSE;
    frame.m_payload = nullptr;
    frame.m_payloadSize = static_cast<uint32_t>(encoded.size());
    return queueControl(peer, frame, encoded);
}

bool NetGatewayNode::select(uint64_t peer, uint32_t product, uint64_t instance, uint64_t now,
    NetGatewayNodeSelection& output) const
{
    NetGatewayNodeSelection best = {};
    uint64_t bestLoad = UINT64_MAX;
    // 路由总数有硬上限；每次选择只聚合一遍，避免每个远端实例都扫描全部路由。
    std::map<std::pair<uint64_t, uint64_t>, uint32_t> reservations;
    for (const auto& route : m_routes)
    {
        const auto target = m_peers.find(route.second.m_service);
        if (target != m_peers.end() && target->second.m_role == NI_LINK_NODE &&
            (route.second.m_pending || route.second.m_loadRevision > target->second.m_snapshotRevision))
        {
            ++reservations[std::make_pair(route.second.m_service, route.second.m_instance)];
        }
    }
    for (const auto& item : m_peers)
    {
        const NetGatewayNodePeer& candidate = item.second;
        if (item.first == peer || candidate.m_closing) { continue; }
        if (candidate.m_serviceReady && candidate.m_product == product && candidate.m_active < candidate.m_capacity &&
            (instance == 0 || (item.first & ~g_nodeLocalPeer) == instance))
        {
            const uint64_t load = (static_cast<uint64_t>(candidate.m_active) + 1) * 1000000000 /
                (static_cast<uint64_t>(candidate.m_capacity) * candidate.m_weight);
            if (load < bestLoad)
            {
                bestLoad = load;
                best.m_peer = item.first;
                best.m_instance = item.first & ~g_nodeLocalPeer;
                best.m_boot = m_boot;
                best.m_window = candidate.m_window;
            }
        }
        if (instance != 0 || candidate.m_role != NI_LINK_NODE || candidate.m_snapshotReceived == 0 ||
            now < candidate.m_snapshotReceived || now - candidate.m_snapshotReceived >= 7000 || candidate.m_active >= 512) { continue; }
        for (const auto& service : candidate.m_remoteServices)
        {
            if (service.m_product != product) { continue; }
            const auto reservation = reservations.find(std::make_pair(item.first, service.m_instance));
            const uint64_t active = static_cast<uint64_t>(service.m_active) +
                (reservation == reservations.end() ? 0 : reservation->second);
            if (active >= service.m_capacity) { continue; }
            const uint64_t load = (active + 1) * 1000000000 /
                (static_cast<uint64_t>(service.m_capacity) * service.m_weight);
            if (load < bestLoad)
            {
                bestLoad = load;
                best.m_peer = item.first;
                best.m_instance = service.m_instance;
                best.m_boot = candidate.m_node.m_boot;
                best.m_window = service.m_window;
            }
        }
    }
    if (best.m_peer == 0) { return false; }
    output = best;
    return true;
}

int32_t NetGatewayNode::open(uint64_t peer, const NetInterfaceFrame& frame, bool mesh)
{
    NetGatewayNodePeer& source = m_peers.find(peer)->second;
    ProtoMessage request;
    uint64_t product = 0;
    uint64_t channel = 0;
    if (frame.m_payloadSize == 0 || !request.from(std::string(reinterpret_cast<const char*>(frame.m_payload), frame.m_payloadSize)) ||
        !GatewayNodeNumber(request, NI_GATEWAY_TARGET_PRODUCT, product) || product > UINT32_MAX ||
        !GatewayNodeNumber(request, NI_GATEWAY_CHANNEL, channel) || channel == 0 || channel >= g_nodeLocalPeer ||
        channel <= source.m_highestChannel)
    {
        return reply(peer, frame, NI_INVALID_ARGUMENT, "");
    }
    uint64_t sourceRole = source.m_role;
    // A valid OPEN owns its number even when no instance can accept it. Its
    // asynchronous caller may already be sending CLOSE before seeing rejection.
    // The per-generation watermark survives arbitrary connection lifetime.
    const auto rejectOpen = [&](int32_t reason) -> int32_t
    {
        const int32_t sent = reply(peer, frame, reason, "");
        if (sent == NI_OK)
        {
            source.m_highestChannel = channel;
            source.m_channelHistory.remember(channel);
        }
        return sent;
    };
    uint64_t requestedInstance = 0;
    uint64_t requestedBoot = 0;
    if (mesh)
    {
        if (source.m_role != NI_LINK_NODE || !GatewayNodeNumber(request, NI_GATEWAY_SOURCE_ROLE, sourceRole) ||
            (sourceRole != NI_LINK_CLIENT && sourceRole != NI_LINK_SERVICE) ||
            !GatewayNodeNumber(request, NI_GATEWAY_INSTANCE, requestedInstance) || requestedInstance == 0 ||
            !GatewayNodeNumber(request, NI_GATEWAY_BOOT, requestedBoot) || requestedBoot != m_boot ||
            ((channel & g_nodeOutboundPeer) != 0) != (source.m_name > m_name)) { return rejectOpen(NI_PROTOCOL_ERROR); }
    }
    else if (source.m_role == NI_LINK_NODE) { return rejectOpen(NI_NOT_SUPPORTED); }
    if (product == 0 && sourceRole != NI_LINK_SERVICE) { return rejectOpen(NI_NOT_SUPPORTED); }
    if (source.m_opened >= (source.m_role == NI_LINK_NODE ? 512U : 32U) || m_routes.size() >= 4096 || m_lastRoute == g_nodeLocalPeer - 1)
    {
        return rejectOpen(NI_LIMIT_EXCEEDED);
    }
    uint32_t requestedWindow = 0;
    if (NetInterfaceCredit::window(request, requestedWindow) != NI_OK)
    {
        return rejectOpen(NI_PROTOCOL_ERROR);
    }
    const uint64_t now = GatewayNodeNow();
    NetGatewayNodeSelection choice = {};
    if ((mesh && m_snapshotRevision == UINT64_MAX) || !select(peer, static_cast<uint32_t>(product),
        requestedInstance, now, choice)) { return rejectOpen(NI_NOT_FOUND); }
    const uint64_t selected = choice.m_peer;
    const uint64_t instance = choice.m_instance;
    const uint64_t boot = choice.m_boot;
    const uint32_t selectedWindow = (std::min)(requestedWindow, choice.m_window);
    if (!controlSpace(selected, 4096) || !controlSpace(peer, 4096)) { return NI_LIMIT_EXCEEDED; }
    const uint64_t routeId = m_lastRoute + 1;
    NetGatewayNodeRoute route;
    route.m_clientWindow.configure(selectedWindow);
    route.m_serviceWindow.configure(selectedWindow);
    route.m_client = peer;
    route.m_clientChannel = channel;
    route.m_service = selected;
    route.m_serviceChannel = routeId | g_nodeLocalPeer;
    route.m_product = static_cast<uint32_t>(product);
    route.m_instance = instance;
    route.m_boot = boot;
    route.m_original = frame;
    route.m_original.m_payload = nullptr;
    route.m_original.m_payloadSize = 0;
    NetGatewayNodePeer& destination = m_peers.find(selected)->second;
    ProtoMessage notice;
    notice.setKeyValue(NI_GATEWAY_WINDOW, Variant(static_cast<int32_t>(selectedWindow)));
    notice.setKeyValue(NI_GATEWAY_TARGET_PRODUCT, Variant(product));
    notice.setKeyValue(NI_GATEWAY_CHANNEL, Variant(route.m_serviceChannel));
    notice.setKeyValue(NI_GATEWAY_SOURCE_ROLE, Variant(static_cast<int32_t>(sourceRole)));
    std::string payload;
    if (!notice.toString(payload)) { return NI_PROTOCOL_ERROR; }
    NetInterfaceFrame event = {};
    event.m_kind = NI_EVENT;
    event.m_messageType = NI_GATEWAY_SERVICE_OPEN;
    if (destination.m_role == NI_LINK_NODE)
    {
        if (destination.m_outRequest == UINT64_MAX || destination.m_outChannel >= g_nodeOutboundPeer - 1) { return rejectOpen(NI_LIMIT_EXCEEDED); }
        route.m_serviceChannel = destination.m_outChannel + 1;
        if (m_name > destination.m_name) { route.m_serviceChannel |= g_nodeOutboundPeer; }
        route.m_remoteRequest = destination.m_outRequest + 1;
        route.m_pending = true;
        route.m_deadline = now + 5000;
        event.m_kind = NI_REQUEST;
        event.m_messageType = NI_GATEWAY_MESH_OPEN;
        event.m_requestId = route.m_remoteRequest;
        notice.setKeyValue(NI_GATEWAY_CHANNEL, Variant(route.m_serviceChannel));
        notice.setKeyValue(NI_GATEWAY_INSTANCE, Variant(instance));
        notice.setKeyValue(NI_GATEWAY_BOOT, Variant(boot));
    }
    if (!notice.toString(payload)) { return NI_PROTOCOL_ERROR; }
    ProtoMessage accepted;
    accepted.setKeyValue(NI_GATEWAY_WINDOW, Variant(static_cast<int32_t>(selectedWindow)));
    accepted.setKeyValue(NI_GATEWAY_CHANNEL, Variant(channel));
    accepted.setKeyValue(NI_GATEWAY_INSTANCE, Variant(instance));
    accepted.setKeyValue(NI_GATEWAY_BOOT, Variant(boot));
    accepted.setKeyValue(NI_GATEWAY_NODE_ID, Variant((route.m_pending ? destination.m_name : m_name).c_str()));
    // 本执行者之后产生的快照才包含本次接纳；OPEN回应之前的旧快照不能解除预留。
    if (mesh) { accepted.setKeyValue(NI_GATEWAY_LOAD_REVISION, Variant(m_snapshotRevision + 1)); }
    std::string response;
    if (!accepted.toString(response)) { return NI_PROTOCOL_ERROR; }
    // 先完整登记路由及全部索引，再提交水位和容量；部分分配失败不能留下孤儿索引，
    // 也不能让随后关闭未完成的路由错误扣减同服务上其他连接的容量。
    const auto clientKey = std::make_pair(peer, channel);
    const auto serviceKey = std::make_pair(selected, route.m_serviceChannel);
    const auto pendingKey = std::make_pair(selected, route.m_remoteRequest);
    bool insertedRoute = false;
    bool insertedClient = false;
    bool insertedService = false;
    bool insertedPending = false;
    int32_t registration = NI_INVALID_STATE;
    try
    {
        insertedRoute = m_routes.insert(std::make_pair(routeId, route)).second;
        if (insertedRoute) { insertedClient = m_channels.insert(std::make_pair(clientKey, routeId)).second; }
        if (insertedClient) { insertedService = m_channels.insert(std::make_pair(serviceKey, routeId)).second; }
        if (insertedService && route.m_pending)
        {
            insertedPending = m_pendingOpens.insert(std::make_pair(pendingKey, routeId)).second;
        }
        if (insertedService && (!route.m_pending || insertedPending)) { registration = NI_OK; }
    }
    catch (...)
    {
        registration = NI_NO_MEMORY;
    }
    if (registration != NI_OK)
    {
        if (insertedPending) { m_pendingOpens.erase(pendingKey); }
        if (insertedService) { m_channels.erase(serviceKey); }
        if (insertedClient) { m_channels.erase(clientKey); }
        if (insertedRoute) { m_routes.erase(routeId); }
        return registration;
    }
    // 两侧控制空间已预留；提交后只接纳一次，flush不再选实例或重复SERVICE_OPEN。
    m_lastRoute = routeId;
    source.m_highestChannel = channel;
    source.m_channelHistory.remember(channel);
    destination.m_channelHistory.remember(route.m_serviceChannel);
    ++source.m_opened;
    ++m_peers.find(selected)->second.m_active;
    if (route.m_pending)
    {
        ++destination.m_outChannel;
        destination.m_outRequest = route.m_remoteRequest;
    }
    m_meshDirty = true;
    if (queueControl(selected, event, payload) != NI_OK || (!route.m_pending && reply(peer, frame, NI_OK, response) != NI_OK))
    {
        retire(routeId, 0, NI_NO_MEMORY);
        close(peer, NI_NO_MEMORY);
    }
    return NI_OK;
}

bool NetGatewayNode::onLinkControl(uint64_t peer, const NetInterfaceFrame& frame)
{
    if (frame.m_kind == NI_EVENT && frame.m_messageType == NI_GATEWAY_NODE_SNAPSHOT) { return snapshot(peer, frame); }
    if (frame.m_kind == NI_EVENT && frame.m_messageType == NI_GATEWAY_CHANNEL_CLOSE) { return meshClose(peer, frame); }
    if (frame.m_kind == NI_RESPONSE) { return meshResponse(peer, frame); }
    if (frame.m_kind == NI_EVENT && frame.m_messageType == NI_GATEWAY_CREDIT) { return credit(peer, frame); }
    auto source = m_peers.find(peer);
    if (source == m_peers.end() || source->second.m_closing || frame.m_productId != 0 || frame.m_kind != NI_REQUEST ||
        frame.m_requestId == 0 || frame.m_requestId <= source->second.m_request)
    {
        close(peer, NI_PROTOCOL_ERROR);
        return true;
    }
    if (!controlSpace(peer, 4096)) { return false; }
    int32_t sent = NI_OK;
    if (frame.m_messageType == NI_GATEWAY_DISCOVER && frame.m_payloadSize == 0)
    {
        std::string bytes;
        if (directory(bytes) != NI_OK) { close(peer, NI_PROTOCOL_ERROR); return true; }
        sent = reply(peer, frame, NI_OK, bytes);
    }
    else if (frame.m_messageType == NI_GATEWAY_SERVICE_READY)
    {
        ProtoMessage ready;
        uint64_t capacity = 0;
        uint64_t weight = 0;
        uint32_t window = 0;
        if (source->second.m_role != NI_LINK_SERVICE || source->second.m_serviceReady || frame.m_payloadSize == 0 ||
            !ready.from(std::string(reinterpret_cast<const char*>(frame.m_payload), frame.m_payloadSize)) ||
            !GatewayNodeNumber(ready, NI_GATEWAY_CAPACITY, capacity) || capacity == 0 || capacity > 4096 ||
            NetInterfaceCredit::window(ready, window) != NI_OK || !GatewayNodeNumber(ready, NI_GATEWAY_WEIGHT, weight) || weight == 0 || weight > 1000)
        {
            sent = reply(peer, frame, NI_INVALID_ARGUMENT, "");
        }
        else
        {
            sent = reply(peer, frame, NI_OK, "");
            if (sent == NI_OK)
            {
                source->second.m_capacity = static_cast<uint32_t>(capacity);
                source->second.m_window = window;
                source->second.m_weight = static_cast<uint32_t>(weight);
                source->second.m_serviceReady = true;
                m_meshDirty = true;
            }
        }
    }
    else if (frame.m_messageType == NI_GATEWAY_OPEN) { sent = open(peer, frame); }
    else if (frame.m_messageType == NI_GATEWAY_MESH_OPEN)
    {
        sent = source->second.m_role == NI_LINK_NODE ? open(peer, frame, true) : reply(peer, frame, NI_NOT_SUPPORTED, "");
    }
    else { sent = reply(peer, frame, NI_NOT_SUPPORTED, ""); }
    if (sent == NI_LIMIT_EXCEEDED) { return false; }
    if (sent != NI_OK) { close(peer, sent); }
    else { source->second.m_request = frame.m_requestId; }
    return true;
}

bool NetGatewayNode::credit(uint64_t peer, const NetInterfaceFrame& frame)
{
    ProtoMessage message;
    uint64_t channel = 0;
    uint64_t amount = 0;
    if (m_peers.count(peer) == 0 || frame.m_productId != 0 || frame.m_requestId != 0 || frame.m_payloadSize == 0 ||
        !message.from(std::string(reinterpret_cast<const char*>(frame.m_payload), frame.m_payloadSize)) ||
        !GatewayNodeNumber(message, NI_GATEWAY_CHANNEL, channel) || !GatewayNodeNumber(message, NI_GATEWAY_CREDIT_BYTES, amount))
    {
        close(peer, NI_PROTOCOL_ERROR);
        return true;
    }
    const auto indexed = m_channels.find(std::make_pair(peer, channel));
    if (indexed == m_channels.end())
    {
        if (!retired(peer, channel)) { close(peer, NI_PROTOCOL_ERROR); }
        return true;
    }
    NetGatewayNodeRoute& route = m_routes.find(indexed->second)->second;
    const bool fromService = route.m_service == peer;
    NetInterfaceCredit& window = fromService ? route.m_clientWindow : route.m_serviceWindow;
    if (amount == 0 || amount > window.capacity() - window.available()) { close(peer, NI_PROTOCOL_ERROR); return true; }
    const uint64_t destination = fromService ? route.m_client : route.m_service;
    message.setKeyValue(NI_GATEWAY_CHANNEL, Variant(fromService ? route.m_clientChannel : route.m_serviceChannel));
    std::string bytes;
    if (!message.toString(bytes)) { close(peer, NI_PROTOCOL_ERROR); return true; }
    if (!controlSpace(destination, static_cast<uint32_t>(bytes.size()))) { return false; }
    const int32_t queued = queueControl(destination, frame, bytes);
    if (queued != NI_OK || window.grant(static_cast<uint32_t>(amount)) != NI_OK) { close(peer, NI_PROTOCOL_ERROR); }
    return true;
}

int32_t NetGatewayNode::sendRecord(uint64_t peer, const NetInterfaceTunnelRecord& record)
{
    if ((peer & g_nodeLocalPeer) != 0) { return m_local.sendRecord(peer & ~g_nodeLocalPeer, record); }
    if ((peer & g_nodeOutboundPeer) != 0)
    {
        const auto found = m_outbound.find(peer);
        return found == m_outbound.end() ? NI_NOT_FOUND : found->second->sendRecord(record);
    }
    return m_transport.sendRecord(peer, record);
}

int32_t NetGatewayNode::sendControl(uint64_t peer, const NetInterfaceFrame& frame)
{
    if ((peer & g_nodeLocalPeer) != 0) { return m_local.sendControl(peer & ~g_nodeLocalPeer, frame); }
    if ((peer & g_nodeOutboundPeer) != 0)
    {
        const auto found = m_outbound.find(peer);
        return found == m_outbound.end() ? NI_NOT_FOUND : found->second->sendControl(frame);
    }
    return m_transport.sendControl(peer, frame);
}

void NetGatewayNode::close(uint64_t peer, int32_t reason)
{
    auto found = m_peers.find(peer);
    if (found != m_peers.end()) { found->second.m_closing = true; }
    if ((peer & g_nodeLocalPeer) != 0) { m_local.close(peer & ~g_nodeLocalPeer, reason); }
    else if ((peer & g_nodeOutboundPeer) != 0)
    {
        const auto connection = m_outbound.find(peer);
        if (connection != m_outbound.end()) { connection->second->close(reason); }
    }
    else { m_transport.close(peer, reason); }
}

bool NetGatewayNode::retired(uint64_t peer, uint64_t channel) const
{
    const auto found = m_peers.find(peer);
    return found != m_peers.end() && found->second.m_channelHistory.obsolete(channel);
}

bool NetGatewayNode::onLinkRecord(uint64_t peer, const NetInterfaceTunnelRecord& record)
{
    auto indexed = m_channels.find(std::make_pair(peer, record.m_channel));
    if (indexed == m_channels.end())
    {
        // 活跃索引优先；只丢弃无法再次分配的旧编号，不给它建立路由或授予额度。
        if (!retired(peer, record.m_channel)) { close(peer, NI_PROTOCOL_ERROR); }
        return true;
    }
    auto routed = m_routes.find(indexed->second);
    if (routed == m_routes.end() || routed->second.m_product != record.m_product)
    {
        close(peer, NI_PROTOCOL_ERROR);
        return true;
    }
    if (routed->second.m_pending) { close(peer, NI_PROTOCOL_ERROR); return true; }
    NetGatewayNodeRoute& route = routed->second;
    const bool forward = route.m_client == peer;
    const uint64_t destination = forward ? route.m_service : route.m_client;
    NetInterfaceTunnelRecord outgoing = record;
    outgoing.m_channel = forward ? route.m_serviceChannel : route.m_clientChannel;
    NetInterfaceCredit& window = forward ? routed->second.m_clientWindow : routed->second.m_serviceWindow;
    if (record.m_kind == NI_TUNNEL_BYTES && record.m_size > window.available())
    {
        close(peer, NI_PROTOCOL_ERROR);
        return true;
    }
    if (record.m_kind == NI_TUNNEL_CLOSE)
    {
        retire(indexed->second, peer, NI_TRUNCATED);
        return true;
    }
    std::deque<std::vector<uint8_t>>& queue = forward ? route.m_forwardQueue : route.m_reverseQueue;
    const int32_t sent = queue.empty() ? sendRecord(destination, outgoing) : NI_LIMIT_EXCEEDED;
    if (sent == NI_LIMIT_EXCEEDED)
    {
        if (queue.size() >= 64 || m_recordBytes + record.m_size > 8 * 1024 * 1024)
        {
            retire(indexed->second, 0, NI_LIMIT_EXCEEDED);
            return true;
        }
        queue.push_back(std::vector<uint8_t>(record.m_data, record.m_data + record.m_size));
        m_recordBytes += record.m_size;
        uint64_t& blocked = forward ? route.m_forwardBlocked : route.m_reverseBlocked;
        if (blocked == 0) { blocked = GatewayNodeNow(); }
    }
    else if (sent != NI_OK) { retire(indexed->second, 0, sent); return true; }
    if (record.m_kind == NI_TUNNEL_BYTES && window.sent(record.m_size) != NI_OK) { close(peer, NI_PROTOCOL_ERROR); return true; }
    return true;
}

void NetGatewayNode::flushRecords()
{
    // 所有积压字节由接纳／弹出／retire成对计数；无积压时不扫描已建立的通道。
    if (m_recordBytes == 0) { return; }
    const uint64_t now = GatewayNodeNow();
    for (auto item = m_routes.begin(); item != m_routes.end();)
    {
        const uint64_t id = item->first;
        NetGatewayNodeRoute& route = (item++)->second;
        for (uint32_t direction = 0; direction < 2; ++direction)
        {
            auto& queue = direction == 0 ? route.m_forwardQueue : route.m_reverseQueue;
            uint64_t& blocked = direction == 0 ? route.m_forwardBlocked : route.m_reverseBlocked;
            if (queue.empty()) { blocked = 0; continue; }
            NetInterfaceTunnelRecord record = {};
            record.m_kind = NI_TUNNEL_BYTES;
            record.m_product = route.m_product;
            record.m_channel = direction == 0 ? route.m_serviceChannel : route.m_clientChannel;
            record.m_data = queue.front().data();
            record.m_size = static_cast<uint32_t>(queue.front().size());
            const int32_t sent = sendRecord(direction == 0 ? route.m_service : route.m_client, record);
            if (sent == NI_OK)
            {
                m_recordBytes -= record.m_size;
                queue.pop_front();
                blocked = queue.empty() ? 0 : now;
            }
            else if (sent != NI_LIMIT_EXCEEDED || now - blocked >= 5000)
            {
                retire(id, 0, sent == NI_LIMIT_EXCEEDED ? NI_TIMEOUT : sent);
                break;
            }
        }
    }
}

void NetGatewayNode::retire(uint64_t id, uint64_t sender, int32_t reason)
{
    const auto found = m_routes.find(id);
    if (found == m_routes.end()) { return; }
    // 只快照通知需要的标量，不能为关闭操作复制含正文的deque或依赖其移动分配。
    const NetGatewayNodeRoute& route = found->second;
    const uint64_t peers[2] = {route.m_client, route.m_service};
    const uint64_t channels[2] = {route.m_clientChannel, route.m_serviceChannel};
    const bool pending = route.m_pending;
    const uint64_t remoteRequest = route.m_remoteRequest;
    const NetInterfaceFrame original = route.m_original;
    for (const auto& bytes : route.m_forwardQueue) { m_recordBytes -= static_cast<uint32_t>(bytes.size()); }
    for (const auto& bytes : route.m_reverseQueue) { m_recordBytes -= static_cast<uint32_t>(bytes.size()); }
    m_routes.erase(found);
    m_channels.erase(std::make_pair(peers[0], channels[0]));
    m_channels.erase(std::make_pair(peers[1], channels[1]));
    if (pending) { m_pendingOpens.erase(std::make_pair(peers[1], remoteRequest)); }
    const auto client = m_peers.find(peers[0]);
    const auto service = m_peers.find(peers[1]);
    if (client != m_peers.end() && client->second.m_opened != 0) { --client->second.m_opened; }
    if (service != m_peers.end() && service->second.m_active != 0) { --service->second.m_active; }
    m_meshDirty = true;
    // 所有内部资源与额度先无分配归还。通知无法分配时关闭对应物理代次，
    // 不得从终态回调抛出后留下半清理路由或让另一侧继续使用旧通道。
    if (pending && !m_stopping && sender != peers[0] && sender != UINT64_MAX &&
        client != m_peers.end() && !client->second.m_closing)
    {
        try
        {
            if (reply(peers[0], original, reason < 0 ? reason : NI_TRUNCATED, "") != NI_OK)
            {
                close(peers[0], NI_LIMIT_EXCEEDED);
            }
        }
        catch (...)
        {
            close(peers[0], NI_NO_MEMORY);
        }
    }
    for (uint32_t index = 0; index < 2; ++index)
    {
        if (m_stopping || sender == UINT64_MAX || peers[index] == sender || (pending && index == 0)) { continue; }
        const auto peer = m_peers.find(peers[index]);
        if (peer == m_peers.end() || peer->second.m_closing) { continue; }
        try
        {
            ProtoMessage message;
            message.setKeyValue(NI_GATEWAY_CHANNEL, Variant(channels[index]));
            message.setKeyValue(NI_GATEWAY_REASON, Variant(reason));
            std::string bytes;
            NetInterfaceFrame event = {};
            event.m_kind = NI_EVENT;
            event.m_messageType = NI_GATEWAY_CHANNEL_CLOSE;
            if (!message.toString(bytes) || queueControl(peers[index], event, bytes) != NI_OK)
            {
                close(peers[index], NI_LIMIT_EXCEEDED);
            }
        }
        catch (...)
        {
            close(peers[index], NI_NO_MEMORY);
        }
    }
}

void NetGatewayNode::onLinkClosed(uint64_t peer, int32_t reason)
{
    const auto owner = m_peers.find(peer);
    if (owner != m_peers.end()) { owner->second.m_closing = true; }
    // retire只删除当前路由并标记其他物理连接；先递增迭代器即可无分配遍历。
    for (auto item = m_routes.begin(); item != m_routes.end();)
    {
        const auto current = item++;
        if (current->second.m_client == peer || current->second.m_service == peer)
        {
            retire(current->first, peer, reason < 0 ? reason : NI_TRUNCATED);
        }
    }
    m_peers.erase(peer);
    m_meshDirty = true;
}

#if 0 // NODE_LOAD_UNIT_TEST_BEGIN
#ifdef _WIN32
#include <windows.h>
#endif
#include "NetGatewayNodeLoadTest.h"
#include <cstdio>

#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
// 仅独占单测main使用；非负数为下一次拒绝分配前允许的普通CRT分配次数。
static int32_t g_nodeAllocationBudget = -1;

/** 对真实节点路由路径逐次拒绝普通堆分配，不修改生产分配器
@param [in] operation CRT分配操作
@param [in] block 堆块类型，只对普通业务堆块注入失败
@param [in] pointer CRT传入的堆块，只在钩子返回前借用；本用例未使用时忽略
@param [in] size CRT请求的字节数，是否参与注入判定由本用例决定
@param [in] request CRT分配序号，本用例不使用
@param [in] filename CRT传入的来源文件名，本用例不读取
@param [in] line 来源代码行号，本用例不使用
@return 1允许，0拒绝；测试退出临界区立即恢复原钩子
*/
static int __cdecl GatewayNodeAllocationHook(int operation, void* pointer, size_t size, int block, long request, const unsigned char* filename, int line)
{
    (void)pointer;
    (void)size;
    (void)request;
    (void)filename;
    (void)line;
    if (block == _NORMAL_BLOCK && (operation == _HOOK_ALLOC || operation == _HOOK_REALLOC))
    {
        if (g_nodeAllocationBudget == 0) { return 0; }
        if (g_nodeAllocationBudget > 0) { --g_nodeAllocationBudget; }
    }
    return 1;
}

#endif

int NetGatewayNodeLoadTest::run()
{
    uint32_t checks = 0;
    uint32_t failures = 0;
    const auto check = [&](bool pass, const char* name)
    {
        ++checks;
        if (!pass) { ++failures; std::printf("FAIL %s\n", name); }
    };
    NetGatewayNode node("node.test", "", "");
    node.m_boot = 53;
    NetGatewayNodePeer remote;
    remote.m_role = NI_LINK_NODE;
    remote.m_snapshotReceived = 100;
    remote.m_snapshotRevision = 10;
    remote.m_node.m_boot = 79;
    NetGatewayNodeService service;
    service.m_instance = 7;
    service.m_product = 101;
    service.m_capacity = 8;
    service.m_weight = 1;
    service.m_window = 131072;
    remote.m_remoteServices.push_back(service);
    node.m_peers.insert(std::make_pair(2, remote));
    NetGatewayNodeSelection selected = {};
    check(node.select(1, 101, 0, 100, selected) && selected.m_peer == 2 && selected.m_instance == 7 &&
        selected.m_boot == 79 && selected.m_window == 131072, "authenticated remote values");
    selected.m_peer = 999;
    check(!node.select(1, 102, 0, 100, selected) && selected.m_peer == 999, "product isolation and atomic no-result");
    check(!node.select(2, 101, 0, 100, selected), "no route back to requester");
    check(!node.select(1, 101, 7, 100, selected), "mesh exact instance cannot hop again");
    check(node.select(1, 101, 0, 7099, selected), "snapshot just inside TTL");
    check(!node.select(1, 101, 0, 7100, selected), "expired snapshot excluded");
    check(!node.select(1, 101, 0, 99, selected), "future snapshot time excluded");
    node.m_peers[2].m_closing = true;
    check(!node.select(1, 101, 0, 100, selected), "closing node excluded");
    node.m_peers[2].m_closing = false;
    node.m_peers[2].m_active = 512;
    check(!node.select(1, 101, 0, 100, selected), "node link hard capacity");
    node.m_peers[2].m_active = 0;
    node.m_peers[2].m_remoteServices[0].m_capacity = 3;
    node.m_peers[2].m_remoteServices[0].m_active = 1;
    NetGatewayNodeRoute reservation;
    reservation.m_service = 2;
    reservation.m_instance = 7;
    reservation.m_pending = true;
    node.m_routes.insert(std::make_pair(1, reservation));
    node.m_routes.insert(std::make_pair(2, reservation));
    check(!node.select(1, 101, 0, 100, selected), "pending admissions consume hard capacity");
    node.m_routes[1].m_pending = node.m_routes[2].m_pending = false;
    node.m_routes[1].m_loadRevision = node.m_routes[2].m_loadRevision = 11;
    check(!node.select(1, 101, 0, 100, selected), "acknowledged admissions retained before snapshot");
    node.m_peers[2].m_snapshotRevision = 11;
    node.m_peers[2].m_remoteServices[0].m_active = 3;
    check(!node.select(1, 101, 0, 100, selected), "new snapshot at hard capacity");
    node.m_peers[2].m_remoteServices[0].m_active = 2;
    check(node.select(1, 101, 0, 100, selected), "reflected admissions no longer double counted");
    node.m_peers[2].m_snapshotRevision = 10;
    node.m_routes[1].m_instance = node.m_routes[2].m_instance = 8;
    check(node.select(1, 101, 0, 100, selected), "reservations scoped to target instance");
    node.m_routes[1].m_service = node.m_routes[2].m_service = 3;
    node.m_routes[1].m_instance = node.m_routes[2].m_instance = 7;
    check(node.select(1, 101, 0, 100, selected), "reservations scoped to node generation");
    node.m_routes.clear();
    node.m_peers[2].m_remoteServices[0] = service;
    node.m_peers.insert(std::make_pair(3, remote));
    uint32_t admitted[2] = {};
    for (uint64_t route = 1; route <= 12; ++route)
    {
        const bool found = node.select(1, 101, 0, 100, selected);
        check(found && (selected.m_peer == 2 || selected.m_peer == 3), "burst selects healthy target");
        if (!found) { break; }
        ++admitted[selected.m_peer - 2];
        reservation.m_service = selected.m_peer;
        reservation.m_instance = selected.m_instance;
        reservation.m_pending = false;
        reservation.m_loadRevision = 11;
        node.m_routes.insert(std::make_pair(route, reservation));
    }
    check(admitted[0] == 6 && admitted[1] == 6, "burst acknowledgements spread before snapshots");
    node.m_routes.clear();
    node.m_peers[2].m_remoteServices[0].m_active = 6;
    node.m_peers[3].m_remoteServices[0].m_active = 1;
    check(node.select(1, 101, 0, 100, selected) && selected.m_peer == 3, "prefer spare capacity");
    node.m_peers[2].m_remoteServices[0].m_active = 0;
    node.m_peers[3].m_remoteServices[0].m_active = 0;
    node.m_peers[3].m_remoteServices[0].m_weight = 2;
    check(node.select(1, 101, 0, 100, selected) && selected.m_peer == 3, "relative weight affects selection");
    node.m_peers.clear();
    NetGatewayNodePeer local;
    local.m_role = NI_LINK_SERVICE;
    local.m_serviceReady = true;
    local.m_product = 101;
    local.m_capacity = 8;
    local.m_window = 65536;
    node.m_peers.insert(std::make_pair(g_nodeLocalPeer | 11, local));
    check(node.select(1, 101, 11, 100, selected) && selected.m_instance == 11 && selected.m_boot == 53 &&
        selected.m_window == 65536, "local exact instance and boot");
    check(!node.select(1, 101, 12, 100, selected), "no substitution for requested instance");
    node.m_peers[g_nodeLocalPeer | 11].m_active = 8;
    check(!node.select(1, 101, 11, 100, selected), "target enforces own hard capacity");
    node.m_peers[g_nodeLocalPeer | 11].m_active = 0;
    remote.m_name = "node.alpha";
    remote.m_remoteServices.clear();
    node.m_peers.insert(std::make_pair(2, remote));
    node.m_snapshotRevision = 10;
    ProtoMessage request;
    request.setKeyValue(NI_GATEWAY_TARGET_PRODUCT, Variant(static_cast<int32_t>(101)));
    request.setKeyValue(NI_GATEWAY_CHANNEL, Variant(UINT64_C(1)));
    request.setKeyValue(NI_GATEWAY_INSTANCE, Variant(UINT64_C(11)));
    request.setKeyValue(NI_GATEWAY_BOOT, Variant(node.m_boot));
    request.setKeyValue(NI_GATEWAY_SOURCE_ROLE, Variant(static_cast<int32_t>(NI_LINK_CLIENT)));
    request.setKeyValue(NI_GATEWAY_WINDOW, Variant(static_cast<int32_t>(65536)));
    std::string payload;
    check(request.toString(payload), "mesh OPEN fixture encoding");
    NetInterfaceFrame frame = {};
    frame.m_kind = NI_REQUEST;
    frame.m_messageType = NI_GATEWAY_MESH_OPEN;
    frame.m_requestId = 1;
    frame.m_payload = reinterpret_cast<const uint8_t*>(payload.data());
    frame.m_payloadSize = static_cast<uint32_t>(payload.size());
    check(node.open(2, frame, true) == NI_OK && node.m_routes.size() == 1 &&
        node.m_peers[g_nodeLocalPeer | 11].m_active == 1, "real OPEN reserves target once");
    ProtoMessage response;
    uint64_t revision = 0;
    check(!node.m_peers[2].m_controls.empty() && response.from(node.m_peers[2].m_controls.back().m_payload) &&
        response.getCode() == NI_OK && GatewayNodeNumber(response, NI_GATEWAY_LOAD_REVISION, revision) &&
        revision == 11, "real mesh ACK carries future load revision");
    // 候选回收属于纯管理状态白盒单测；实际TLS发现及故障转移仍由独占多节点用例验证。
    NetGatewayNode discovery("node.self", "", "");
    check(discovery.addSeed("127.0.0.1", 5203) == NI_OK, "configured seed admitted");
    NetInterfaceDirectoryPoint point;
    point.m_name = "node." + std::string(32, 'a');
    point.m_address = "127.0.0.2";
    point.m_capacity = 8;
    for (uint32_t generation = 0; generation < 130; ++generation)
    {
        point.m_port = 30000 + generation;
        point.m_boot = generation + 1;
        check(discovery.remember(point, true) == NI_OK, "authenticated address change remains admissible");
        check(discovery.m_candidates.size() == 2, "old learned address must not consume a permanent candidate slot");
    }
    check(discovery.addSeed(point.m_address, point.m_port) == NI_OK, "learned endpoint can be promoted to configured seed");
    const uint32_t pinnedPort = point.m_port;
    ++point.m_port;
    ++point.m_boot;
    check(discovery.remember(point, true) == NI_OK && discovery.m_candidates.size() == 3,
        "moving a node retains configured seeds and only the current learned address");
    point.m_name = "node." + std::string(32, 'b');
    check(discovery.remember(point, true) == NI_OK && discovery.m_members.size() == 1 && discovery.m_candidates.size() == 3,
        "new authenticated identity at the same address replaces old membership without candidate growth");
    // 固定白盒单调时钟，调用真实过期分支；不冒称执行过五分钟浸泡。
    discovery.m_members.begin()->second.m_seen = 100;
    discovery.expireMembers(300099);
    check(discovery.m_members.size() == 1 && discovery.m_candidates.size() == 3, "membership just inside expiry stays usable");
    discovery.expireMembers(300100);
    check(discovery.m_members.empty() && discovery.m_candidates.size() == 2 &&
        discovery.m_candidates.count(std::make_pair(std::string("127.0.0.1"), 5203U)) == 1 &&
        discovery.m_candidates.count(std::make_pair(point.m_address, pinnedPort)) == 1,
        "expired learned membership releases its candidate while configured seeds survive");
    // 普通控制队列已满时，批量通道关闭不能终止仍健康的共享服务。
    NetGatewayNode retiring("node.closing", "", "");
    retiring.m_peers[1].m_opened = 40;
    retiring.m_peers[2].m_active = 40;
    NetInterfaceFrame normal = {};
    normal.m_kind = NI_EVENT;
    for (uint32_t index = 0; index < 32; ++index)
    {
        check(retiring.queueControl(2, normal, "") == NI_OK, "prepare full ordinary control budget");
    }
    for (uint32_t index = 1; index <= 40; ++index)
    {
        NetGatewayNodeRoute route;
        route.m_client = 1;
        route.m_clientChannel = index;
        route.m_service = 2;
        route.m_serviceChannel = UINT64_C(0x8000000000000000) | index;
        retiring.m_routes.insert(std::make_pair(index, route));
        retiring.m_channels[std::make_pair(1, route.m_clientChannel)] = index;
        retiring.m_channels[std::make_pair(2, route.m_serviceChannel)] = index;
        retiring.retire(index, 1, NI_TRUNCATED);
        check(!retiring.m_peers[2].m_closing, "closing one channel must retain the shared service");
    }
    check(retiring.m_routes.empty() && retiring.m_channels.empty() && retiring.m_peers[2].m_active == 0 &&
        retiring.m_peers[1].m_opened == 0 && retiring.m_peers[2].m_controls.size() == 72,
        "all channels release capacity and each close remains queued exactly once");
    check(retiring.queueControl(2, normal, "") == NI_LIMIT_EXCEEDED,
        "close reserve cannot admit ordinary work above its original bound");
    NetInterfaceFrame closing = {};
    closing.m_kind = NI_EVENT;
    closing.m_messageType = NI_GATEWAY_CHANNEL_CLOSE;
    check(retiring.queueControl(2, closing, std::string(129, 'x')) == NI_LIMIT_EXCEEDED,
        "close reserve rejects oversized payloads");
    uint32_t extra = 0;
    while (retiring.queueControl(2, closing, "") == NI_OK) { ++extra; }
    check(extra == 4056 && retiring.m_peers[2].m_controls.size() == 4128 &&
        retiring.m_peers[2].m_controlBytes <= 589824,
        "close reserve remains bounded by ordinary controls plus maximum node routes");
    // 4096条已建立且无积压的通道不应在每轮转发／OPEN期限检查中逐条做无用工作。
    // 此处只测管理状态的CPU耗时；真实网络吞吐和故障迁移由NetWorkTest多进程用例验证。
    NetGatewayNode idle("node.idle", "", "");
    idle.m_snapshot = "fixed test snapshot";
    idle.m_snapshotAt = GatewayNodeNow();
    idle.m_meshDirty = false;
    idle.m_lastOutbound = g_nodeOutboundPeer - 1;
    NetGatewayNodeRoute established;
    for (uint64_t id = 1; id <= 4096; ++id) { idle.m_routes.insert(std::make_pair(id, established)); }
#ifdef _WIN32
    LARGE_INTEGER idleBegin = {};
    LARGE_INTEGER frequency = {};
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&idleBegin);
#else
    const auto idleBegin = std::chrono::steady_clock::now();
#endif
    for (uint32_t step = 0; step < 5000; ++step) { idle.flushRecords(); idle.pollMesh(); }
#ifdef _WIN32
    LARGE_INTEGER idleEnd = {};
    QueryPerformanceCounter(&idleEnd);
    const long long idleMicros = (idleEnd.QuadPart - idleBegin.QuadPart) * 1000000 / frequency.QuadPart;
#else
    const long long idleMicros = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - idleBegin).count();
#endif
    check(idle.m_routes.size() == 4096 && idle.m_recordBytes == 0 && idle.m_pendingOpens.empty(),
        "idle polls retain established channels and accounting");
    for (uint64_t id = 4097; id <= 4100; ++id)
    {
        NetGatewayNodeRoute pending;
        pending.m_pending = true;
        pending.m_service = 2;
        pending.m_remoteRequest = id;
        pending.m_deadline = id == 4100 ? UINT64_MAX : 1;
        idle.m_routes.insert(std::make_pair(id, pending));
        idle.m_pendingOpens.insert(std::make_pair(std::make_pair(pending.m_service, id), id));
    }
    idle.pollMesh();
    check(idle.m_routes.size() == 4097 && idle.m_pendingOpens.size() == 1 &&
        idle.m_routes.count(4100) == 1 && idle.m_pendingOpens.begin()->second == 4100,
        "adjacent expired OPENs retire once; future and established routes remain");
    idle.m_routes[4100].m_deadline = 1;
    idle.m_snapshotAt = GatewayNodeNow();
    idle.m_meshDirty = false;
    idle.pollMesh();
    check(idle.m_routes.size() == 4096 && idle.m_pendingOpens.empty(), "last pending OPEN releases its index");
    // 有积压时仍必须推进真实发送错误和清理，不能把快速路径写成无条件跳过。
    NetGatewayNodeRoute broken;
    broken.m_client = 1;
    broken.m_service = 2;
    broken.m_forwardQueue.push_back(std::vector<uint8_t>(16, 7));
    broken.m_reverseQueue.push_back(std::vector<uint8_t>(8, 9));
    idle.m_routes.insert(std::make_pair(4101, broken));
    idle.m_recordBytes = 24;
    idle.flushRecords();
    check(idle.m_routes.size() == 4096 && idle.m_routes.count(4101) == 0 && idle.m_recordBytes == 0,
        "nonempty failed route is retired and both directions release bytes");

#if defined(_MSC_VER) && defined(_DEBUG)
    // 原关闭路径分配临时vector并复制路由；内存紧张时抛出会使终态回调留下孤儿路由。
    NetGatewayNode exhausted("node.oom-close", "", "");
    exhausted.m_peers[1].m_opened = 2;
    exhausted.m_peers[2].m_active = 2;
    for (uint64_t id = 1; id <= 2; ++id)
    {
        NetGatewayNodeRoute route;
        route.m_client = 1;
        route.m_clientChannel = id;
        route.m_service = 2;
        route.m_serviceChannel = g_nodeLocalPeer | id;
        route.m_pending = id == 2;
        route.m_remoteRequest = id;
        route.m_forwardQueue.push_back(std::vector<uint8_t>(16, 1));
        route.m_reverseQueue.push_back(std::vector<uint8_t>(8, 2));
        exhausted.m_routes.insert(std::make_pair(id, route));
        exhausted.m_channels[std::make_pair(1, route.m_clientChannel)] = id;
        exhausted.m_channels[std::make_pair(2, route.m_serviceChannel)] = id;
        if (route.m_pending) { exhausted.m_pendingOpens[std::make_pair(2, id)] = id; }
        exhausted.m_recordBytes += 24;
    }
    bool closeThrew = false;
    g_nodeAllocationBudget = 0;
    _CRT_ALLOC_HOOK previousHook = _CrtSetAllocHook(GatewayNodeAllocationHook);
    try { exhausted.onLinkClosed(1, NI_TRUNCATED); }
    catch (...) { closeThrew = true; }
    _CrtSetAllocHook(previousHook);
    g_nodeAllocationBudget = -1;
    check(!closeThrew && exhausted.m_routes.empty() && exhausted.m_channels.empty() &&
        exhausted.m_pendingOpens.empty() && exhausted.m_recordBytes == 0 && exhausted.m_peers.count(1) == 0 &&
        exhausted.m_peers[2].m_active == 0, "allocation failure cannot interrupt route teardown");
    check(exhausted.m_peers[2].m_closing, "unavailable close notification must terminate its recipient safely");

    uint32_t failedOpenAllocations = 0;
    uint32_t successfulOpenPaths = 0;
    for (uint32_t remoteTarget = 0; remoteTarget < 2; ++remoteTarget)
    for (int32_t budget = 0; budget < 1024; ++budget)
    {
        NetGatewayNode opening("node.oom-open", "", "");
        opening.m_boot = 19;
        opening.m_lastRoute = 1;
        opening.m_peers[1].m_role = NI_LINK_CLIENT;
        opening.m_peers[3].m_opened = 1;
        const uint64_t target = remoteTarget == 0 ? (g_nodeLocalPeer | 11) : 2;
        NetGatewayNodePeer& destination = opening.m_peers[target];
        destination.m_role = NI_LINK_SERVICE;
        destination.m_serviceReady = true;
        destination.m_product = 101;
        destination.m_capacity = 8;
        destination.m_active = 1;
        if (remoteTarget != 0)
        {
            destination.m_role = NI_LINK_NODE;
            destination.m_name = "node.z";
            destination.m_serviceReady = false;
            destination.m_snapshotReceived = GatewayNodeNow();
            destination.m_snapshotRevision = 1;
            destination.m_node.m_boot = 23;
            NetGatewayNodeService offered;
            offered.m_instance = 11;
            offered.m_product = 101;
            offered.m_capacity = 8;
            offered.m_active = 1;
            offered.m_weight = 1;
            destination.m_remoteServices.push_back(offered);
        }
        NetGatewayNodeRoute prior;
        prior.m_client = 3;
        prior.m_clientChannel = 1;
        prior.m_service = target;
        prior.m_serviceChannel = g_nodeLocalPeer | 1;
        opening.m_routes.insert(std::make_pair(1, prior));
        opening.m_channels[std::make_pair(3, prior.m_clientChannel)] = 1;
        opening.m_channels[std::make_pair(target, prior.m_serviceChannel)] = 1;
        ProtoMessage wanted;
        wanted.setKeyValue(NI_GATEWAY_TARGET_PRODUCT, Variant(static_cast<int32_t>(101)));
        wanted.setKeyValue(NI_GATEWAY_CHANNEL, Variant(UINT64_C(1)));
        wanted.setKeyValue(NI_GATEWAY_WINDOW, Variant(static_cast<int32_t>(65536)));
        std::string body;
        check(wanted.toString(body), "allocation OPEN fixture encoding");
        NetInterfaceFrame openingFrame = {};
        openingFrame.m_kind = NI_REQUEST;
        openingFrame.m_messageType = NI_GATEWAY_OPEN;
        openingFrame.m_requestId = 1;
        openingFrame.m_payload = reinterpret_cast<const uint8_t*>(body.data());
        openingFrame.m_payloadSize = static_cast<uint32_t>(body.size());
        g_nodeAllocationBudget = budget;
        previousHook = _CrtSetAllocHook(GatewayNodeAllocationHook);
        try { opening.open(1, openingFrame); }
        catch (...) { }
        const bool rejected = g_nodeAllocationBudget == 0;
        _CrtSetAllocHook(previousHook);
        g_nodeAllocationBudget = -1;
        if (rejected) { ++failedOpenAllocations; }
        bool indexed = opening.m_channels.size() == opening.m_routes.size() * 2;
        for (const auto& route : opening.m_routes)
        {
            indexed = indexed && opening.m_channels.count(std::make_pair(route.second.m_client, route.second.m_clientChannel)) == 1 &&
                opening.m_channels.count(std::make_pair(route.second.m_service, route.second.m_serviceChannel)) == 1;
        }
        check(indexed, "partial OPEN allocation must not publish orphan indexes");
        opening.onLinkClosed(1, NI_NO_MEMORY);
        check(opening.m_routes.size() == 1 && opening.m_routes.count(1) == 1 && opening.m_channels.size() == 2 &&
            opening.m_peers[target].m_active == 1 && opening.m_peers[3].m_opened == 1,
            "failed OPEN teardown preserves unrelated route and capacity");
        if (!rejected) { ++successfulOpenPaths; break; }
    }
    check(successfulOpenPaths == 2, "allocation walk reaches a successful local and remote OPEN");
    check(failedOpenAllocations > 20, "allocation walk exercises actual OPEN allocations");
    std::printf("NODE_ALLOCATION failed_positions=%u\n", failedOpenAllocations);
#endif

    std::printf("NODE_IDLE_TIMING channels=4096 polls=5000 elapsed_us=%lld\n", idleMicros);
    std::printf("NODE_LOAD checks=%u failures=%u %s\n", checks, failures, failures == 0 ? "PASS" : "FAIL");
    return failures == 0 ? 0 : 1;
}

/** 仅在独占单测副本中放开，生产源码始终保持关闭
@return 单测退出码
*/
int main()
{
    return NetGatewayNodeLoadTest::run();
}

#endif // NODE_LOAD_UNIT_TEST_END

NetInterfaceDirectoryPoint NetGatewayNode::self() const
{
    NetInterfaceDirectoryPoint point;
    point.m_name = m_name;
    point.m_address = m_advertise;
    point.m_port = localPort();
    point.m_capacity = m_capacity;
    point.m_boot = m_boot;
    return point;
}

int32_t NetGatewayNode::addSeed(const std::string& address, uint32_t port)
{
    if (m_stopping || !NetInterfaceDirectory::address(address) || port == 0 || port > 65535)
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        const auto key = std::make_pair(address, port);
        auto candidate = m_candidates.find(key);
        if (candidate == m_candidates.end())
        {
            if (m_candidates.size() >= 128) { return NI_LIMIT_EXCEEDED; }
            candidate = m_candidates.insert(std::make_pair(key, NetGatewayNodeCandidate())).first;
        }
        // 允许将已学习的候选提升为固定种子，重复配置不创建额外条目。
        candidate->second.m_seed = true;
        return NI_OK;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetGatewayNode::configureCache(const std::string& path)
{
    if (m_stopping || !m_receiving || m_cacheEnabled) { return NI_INVALID_STATE; }
    const int32_t opened = m_cache.open(path);
    m_cacheEnabled = opened == NI_OK;
    return opened;
}

int32_t NetGatewayNode::remember(const NetInterfaceDirectoryPoint& point, bool verified)
{
    if (!NetInterfaceDirectory::valid(point))
    {
        return NI_PROTOCOL_ERROR;
    }
    if (point.m_name == m_name) { return NI_OK; }
    if (point.m_address == m_advertise && point.m_port == localPort()) { return NI_PROTOCOL_ERROR; }
    const uint64_t now = MeshNow();
    auto found = m_members.find(point.m_name);
    if (found != m_members.end() && !verified) { return NI_OK; }
    for (auto item = m_members.begin(); item != m_members.end();)
    {
        if (item->first != point.m_name && item->second.m_point.m_address == point.m_address && item->second.m_point.m_port == point.m_port)
        {
            if (!verified) { return NI_OK; }
            item = m_members.erase(item);
        }
        else { ++item; }
    }
    found = m_members.find(point.m_name);
    if (found == m_members.end())
    {
        if (m_members.size() >= NET_INTERFACE_DIRECTORY_NODES - 1) { return NI_LIMIT_EXCEEDED; }
        found = m_members.insert(std::make_pair(point.m_name, NetGatewayNodeMember())).first;
    }
    if (found->second.m_point.m_boot != point.m_boot || found->second.m_point.m_address != point.m_address ||
        found->second.m_point.m_port != point.m_port || found->second.m_point.m_capacity != point.m_capacity) { m_cacheDirty = true; }
    found->second.m_point = point;
    found->second.m_seen = now;
    found->second.m_verified = verified;
    // 先更新成员，再裁去其旧地址；学习地址不能被当成永不回收的配置种子。
    pruneCandidates();
    const auto key = std::make_pair(point.m_address, point.m_port);
    auto candidate = m_candidates.find(key);
    if (candidate == m_candidates.end())
    {
        if (m_candidates.size() >= 128) { return NI_LIMIT_EXCEEDED; }
        candidate = m_candidates.insert(std::make_pair(key, NetGatewayNodeCandidate())).first;
    }
    candidate->second.m_expectedName = point.m_name;
    return NI_OK;
}

int32_t NetGatewayNode::directory(std::string& output) const
{
    std::vector<NetInterfaceDirectoryPoint> points;
    points.push_back(self());
    for (const auto& item : m_members) { points.push_back(item.second.m_point); }
    return NetInterfaceDirectory::encode(points, output);
}

int32_t NetGatewayNode::buildSnapshot()
{
    std::vector<NetInterfaceDirectoryPoint> own(1, self());
    std::string bytes;
    if (NetInterfaceDirectory::encode(own, bytes) != NI_OK)
    {
        return NI_PROTOCOL_ERROR;
    }
    ProtoMessage message;
    if (!message.from(bytes)) { return NI_PROTOCOL_ERROR; }
    uint32_t count = 0;
    for (const auto& item : m_peers)
    {
        const NetGatewayNodePeer& peer = item.second;
        if (!peer.m_serviceReady || peer.m_closing || (item.first & g_meshLocal) == 0) { continue; }
        if (++count > 64) { return NI_LIMIT_EXCEEDED; }
        const int32_t group = static_cast<int32_t>(count + 1);
        message.setKeyValue(NI_GATEWAY_INSTANCE, Variant(item.first & ~g_meshLocal), group);
        message.setKeyValue(NI_GATEWAY_TARGET_PRODUCT, Variant(static_cast<uint64_t>(peer.m_product)), group);
        message.setKeyValue(NI_GATEWAY_WINDOW, Variant(static_cast<int32_t>(peer.m_window)), group);
        message.setKeyValue(NI_GATEWAY_CAPACITY, Variant(static_cast<int32_t>(peer.m_capacity)), group);
        message.setKeyValue(NI_GATEWAY_WEIGHT, Variant(static_cast<int32_t>(peer.m_weight)), group);
        message.setKeyValue(NI_GATEWAY_ACTIVE, Variant(static_cast<int32_t>(peer.m_active)), group);
    }
    message.setKeyValue(NI_GATEWAY_SERVICE_COUNT, Variant(static_cast<int32_t>(count)));
    if (!message.toString(bytes) || bytes.size() > 16384 || m_snapshotRevision == UINT64_MAX) { return NI_LIMIT_EXCEEDED; }
    m_snapshot.swap(bytes);
    ++m_snapshotRevision;
    m_snapshotAt = MeshNow();
    m_meshDirty = false;
    return NI_OK;
}

bool NetGatewayNode::snapshot(uint64_t peer, const NetInterfaceFrame& frame)
{
    auto source = m_peers.find(peer);
    std::vector<NetInterfaceDirectoryPoint> own;
    ProtoMessage message;
    uint64_t count = 0;
    if (source == m_peers.end() || source->second.m_role != NI_LINK_NODE || frame.m_kind != NI_EVENT || frame.m_productId != 0 ||
        frame.m_requestId == 0 || frame.m_requestId <= source->second.m_snapshotRevision || frame.m_payloadSize == 0 ||
        NetInterfaceDirectory::parse(frame.m_payload, frame.m_payloadSize, own) != NI_OK || own.size() != 1 ||
        own[0].m_name != source->second.m_name ||
        !message.from(std::string(reinterpret_cast<const char*>(frame.m_payload), frame.m_payloadSize)) ||
        !NetInterfaceDirectory::number(message, NI_GATEWAY_SERVICE_COUNT, count) || count > 64)
    {
        close(peer, NI_PROTOCOL_ERROR);
        return true;
    }
    std::vector<NetGatewayNodeService> services;
    std::set<uint64_t> instances;
    for (int32_t group = 2; group < static_cast<int32_t>(count + 2); ++group)
    {
        NetGatewayNodeService service;
        uint64_t product = 0;
        uint64_t capacity = 0;
        uint64_t weight = 0;
        uint64_t active = 0;
        if (!NetInterfaceDirectory::number(message, NI_GATEWAY_INSTANCE, service.m_instance, group) || service.m_instance == 0 ||
            service.m_instance >= g_meshOutbound || !instances.insert(service.m_instance).second ||
            !NetInterfaceDirectory::number(message, NI_GATEWAY_TARGET_PRODUCT, product, group) || product > UINT32_MAX ||
            !NetInterfaceDirectory::number(message, NI_GATEWAY_CAPACITY, capacity, group) || capacity == 0 || capacity > 4096 ||
            NetInterfaceCredit::window(message, service.m_window, group) != NI_OK || !NetInterfaceDirectory::number(message, NI_GATEWAY_WEIGHT, weight, group) || weight == 0 || weight > 1000 ||
            !NetInterfaceDirectory::number(message, NI_GATEWAY_ACTIVE, active, group) || active > capacity)
        {
            close(peer, NI_PROTOCOL_ERROR);
            return true;
        }
        service.m_product = static_cast<uint32_t>(product);
        service.m_capacity = static_cast<uint32_t>(capacity);
        service.m_weight = static_cast<uint32_t>(weight);
        service.m_active = static_cast<uint32_t>(active);
        services.push_back(service);
    }
    if (remember(own[0], true) != NI_OK) { close(peer, NI_LIMIT_EXCEEDED); return true; }
    source->second.m_node = own[0];
    source->second.m_remoteServices.swap(services);
    source->second.m_snapshotReceived = MeshNow();
    source->second.m_snapshotRevision = frame.m_requestId;
    return true;
}

bool NetGatewayNode::meshResponse(uint64_t peer, const NetInterfaceFrame& frame)
{
    auto source = m_peers.find(peer);
    if (source == m_peers.end() || source->second.m_role != NI_LINK_NODE || frame.m_kind != NI_RESPONSE ||
        frame.m_productId != 0 || frame.m_requestId == 0) { close(peer, NI_PROTOCOL_ERROR); return true; }
    const auto key = std::make_pair(peer, frame.m_requestId);
    if (frame.m_requestId == source->second.m_discoverRequest)
    {
        if (frame.m_messageType != NI_GATEWAY_DISCOVER) { close(peer, NI_PROTOCOL_ERROR); return true; }
        std::vector<NetInterfaceDirectoryPoint> points;
        if (NetInterfaceDirectory::parse(frame.m_payload, frame.m_payloadSize, points) != NI_OK)
        {
            close(peer, NI_PROTOCOL_ERROR); return true;
        }
        for (const auto& point : points)
        {
            const int32_t result = remember(point, false);
            if (result != NI_OK && result != NI_LIMIT_EXCEEDED) { close(peer, result); return true; }
        }
        source->second.m_discoverRequest = 0;
        return true;
    }
    const auto pending = m_pendingOpens.find(key);
    if (pending == m_pendingOpens.end() && frame.m_requestId <= source->second.m_outRequest &&
        (frame.m_messageType == NI_GATEWAY_DISCOVER || frame.m_messageType == NI_GATEWAY_MESH_OPEN))
    {
        // 只丢弃本代次已过期的管理回应；活跃关联、未来号和命令仍严格核验。
        return true;
    }
    if (frame.m_messageType != NI_GATEWAY_MESH_OPEN || pending == m_pendingOpens.end()) { close(peer, NI_PROTOCOL_ERROR); return true; }
    auto found = m_routes.find(pending->second);
    if (found == m_routes.end() || !found->second.m_pending) { close(peer, NI_PROTOCOL_ERROR); return true; }
    NetGatewayNodeRoute& route = found->second;
    ProtoMessage response;
    uint64_t channel = 0;
    uint64_t instance = 0;
    uint64_t boot = 0;
    uint64_t loadRevision = 0;
    std::string node;
    if (frame.m_payloadSize == 0 || !response.from(std::string(reinterpret_cast<const char*>(frame.m_payload), frame.m_payloadSize)))
    {
        close(peer, NI_PROTOCOL_ERROR);
        return true;
    }
    if (response.getCode() == NI_OK &&
        (!NetInterfaceDirectory::number(response, NI_GATEWAY_CHANNEL, channel) || channel != route.m_serviceChannel ||
         !NetInterfaceDirectory::number(response, NI_GATEWAY_INSTANCE, instance) || instance != route.m_instance ||
         !NetInterfaceDirectory::number(response, NI_GATEWAY_BOOT, boot) || boot != route.m_boot ||
         !NetInterfaceDirectory::number(response, NI_GATEWAY_LOAD_REVISION, loadRevision) || loadRevision == 0 ||
         !NetInterfaceDirectory::text(response, NI_GATEWAY_NODE_ID, node, 37) || node != source->second.m_name))
    {
        close(peer, NI_PROTOCOL_ERROR);
        return true;
    }
    uint32_t window = 0;
    if (response.getCode() == NI_OK && (NetInterfaceCredit::window(response, window) != NI_OK ||
        window > route.m_clientWindow.capacity() || route.m_clientWindow.configure(window) != NI_OK ||
        route.m_serviceWindow.configure(window) != NI_OK)) { close(peer, NI_PROTOCOL_ERROR); return true; }
    if (!controlSpace(route.m_client, 4096)) { return false; }
    response.setKeyValue(NI_GATEWAY_CHANNEL, Variant(route.m_clientChannel));
    std::string bytes;
    if (!response.toString(bytes)) { close(peer, NI_PROTOCOL_ERROR); return true; }
    const int32_t sent = reply(route.m_client, route.m_original, response.getCode(), bytes);
    if (sent == NI_LIMIT_EXCEEDED) { return false; }
    if (sent != NI_OK) { close(route.m_client, sent); return true; }
    m_pendingOpens.erase(pending);
    if (response.getCode() == NI_OK)
    {
        route.m_loadRevision = loadRevision;
        route.m_pending = false;
    }
    else { retire(found->first, UINT64_MAX, response.getCode()); }
    return true;
}

bool NetGatewayNode::meshClose(uint64_t peer, const NetInterfaceFrame& frame)
{
    const auto source = m_peers.find(peer);
    ProtoMessage message;
    uint64_t channel = 0;
    Variant reason;
    if (source == m_peers.end() || frame.m_productId != 0 || frame.m_requestId != 0 ||
        frame.m_payloadSize == 0 || !message.from(std::string(reinterpret_cast<const char*>(frame.m_payload), frame.m_payloadSize)) ||
        !NetInterfaceDirectory::number(message, NI_GATEWAY_CHANNEL, channel) ||
        !message.getKeyValue(NI_GATEWAY_REASON, reason) || reason.type() != Variant::INT32)
    {
        close(peer, NI_PROTOCOL_ERROR);
        return true;
    }
    const auto index = m_channels.find(std::make_pair(peer, channel));
    if (index == m_channels.end())
    {
        if (!retired(peer, channel)) { close(peer, NI_PROTOCOL_ERROR); }
    }
    else { retire(index->second, peer, static_cast<int32_t>(reason)); }
    return true;
}

void NetGatewayNode::pollMesh()
{
    for (auto& item : m_outbound) { item.second->poll(); }
    const uint64_t now = MeshNow();
    for (auto item = m_outbound.begin(); item != m_outbound.end();)
    {
        if (!item->second->m_closed) { ++item; continue; }
        const auto candidate = m_candidates.find(std::make_pair(item->second->m_address, item->second->m_port));
        if (candidate != m_candidates.end())
        {
            uint32_t random = 0;
            if (NetTransport::randomBytes(reinterpret_cast<uint8_t*>(&random), sizeof(random)) != NI_OK)
            {
                random = 0;
            }
            candidate->second.m_retryAt = now + candidate->second.m_delay / 2 + random % candidate->second.m_delay;
            candidate->second.m_delay = (std::min)(candidate->second.m_delay * 2, 10000U);
        }
        item->second->shutdown();
        item = m_outbound.erase(item);
    }
    expireMembers(now);
    if (m_cacheEnabled)
    {
        m_cache.poll();
        uint32_t cachedCount = 0;
        if (m_cache.takeLoaded(nullptr, 0, cachedCount) == NI_BUFFER_TOO_SMALL && cachedCount > 0 && cachedCount <= 64)
        {
            std::vector<NetInterfaceDirectoryNodeData> cached(cachedCount);
            for (auto& record : cached)
            {
                record.m_structSize = sizeof(record);
                record.m_version = 1;
            }
            if (m_cache.takeLoaded(cached.data(), cachedCount, cachedCount) == NI_READY)
            {
                for (const auto& record : cached)
                {
                    NetInterfaceDirectoryPoint point;
                    point.m_name = record.m_name;
                    point.m_address = record.m_address;
                    point.m_port = record.m_port;
                    point.m_capacity = record.m_capacity;
                    point.m_boot = record.m_boot;
                    if (point.m_name == m_name || m_members.count(point.m_name) != 0 ||
                        (point.m_address == m_advertise && point.m_port == localPort()))
                    {
                        continue;
                    }
                    // 迟到缓存只补充候选，不能覆盖本轮已有的认证成员或授予可信身份。
                    if (remember(point, false) == NI_LIMIT_EXCEEDED)
                    {
                        break;
                    }
                }
            }
        }
        if ((m_cacheDirty && (m_cacheQueued == 0 || now - m_cacheQueued >= 1000)) || now - m_cacheQueued >= 30000)
        {
            std::vector<NetInterfaceDirectoryPoint> points;
            points.push_back(self());
            for (const auto& item : m_members) { points.push_back(item.second.m_point); }
            if (m_cache.queue(points) == NI_OK) { m_cacheDirty = false; m_cacheQueued = now; }
        }
        if (m_cache.stopped())
        {
            std::fputs("Cannot update nodes.cache; live authenticated discovery remains available\n", stderr);
            m_cacheEnabled = false;
        }
    }
    if (m_snapshot.empty() || now - m_snapshotAt >= 2000 || (m_meshDirty && now - m_snapshotAt >= 100))
    {
        if (buildSnapshot() != NI_OK) { return; }
    }
    for (auto& item : m_peers)
    {
        NetGatewayNodePeer& peer = item.second;
        if (peer.m_role != NI_LINK_NODE || peer.m_closing) { continue; }
        if (peer.m_snapshotSent != m_snapshotRevision && controlSpace(item.first, static_cast<uint32_t>(m_snapshot.size())))
        {
            NetInterfaceFrame frame = {};
            frame.m_kind = NI_EVENT;
            frame.m_messageType = NI_GATEWAY_NODE_SNAPSHOT;
            frame.m_requestId = m_snapshotRevision;
            if (queueControl(item.first, frame, m_snapshot) == NI_OK) { peer.m_snapshotSent = m_snapshotRevision; }
        }
        if (peer.m_discoverRequest != 0 && now - peer.m_discoverAt >= 5000) { close(item.first, NI_TIMEOUT); continue; }
        if (peer.m_discoverRequest == 0 && (peer.m_discoverAt == 0 || now - peer.m_discoverAt >= 2000) && controlSpace(item.first, 0))
        {
            NetInterfaceFrame frame = {};
            frame.m_kind = NI_REQUEST;
            frame.m_messageType = NI_GATEWAY_DISCOVER;
            if (peer.m_outRequest == UINT64_MAX) { close(item.first, NI_LIMIT_EXCEEDED); continue; }
            frame.m_requestId = peer.m_outRequest + 1;
            if (queueControl(item.first, frame, "") == NI_OK)
            {
                peer.m_outRequest = frame.m_requestId;
                peer.m_discoverRequest = frame.m_requestId;
                peer.m_discoverAt = now;
            }
        }
    }
    // OPEN已有精确关联索引，只检查仍等待回应的路由；不逐轮遍历全部已建立通道。
    // retire会删除当前关联，先推进迭代器，避免连续超时时失效或跳项。
    for (auto pending = m_pendingOpens.begin(); pending != m_pendingOpens.end();)
    {
        const auto current = pending++;
        const auto route = m_routes.find(current->second);
        if (route == m_routes.end() || !route->second.m_pending)
        {
            m_pendingOpens.erase(current);
            continue;
        }
        if (now >= route->second.m_deadline) { retire(route->first, 0, NI_TIMEOUT); }
    }
    if (m_outbound.size() >= 64 || m_lastOutbound >= g_meshOutbound - 1) { return; }
    for (auto& item : m_candidates)
    {
        if (now < item.second.m_retryAt || (item.first.first == m_advertise && item.first.second == localPort())) { continue; }
        bool connected = false;
        for (const auto& connection : m_outbound)
        {
            if (connection.second->m_address == item.first.first && connection.second->m_port == item.first.second) { connected = true; break; }
        }
        for (const auto& peer : m_peers)
        {
            if (peer.second.m_role == NI_LINK_NODE && !peer.second.m_closing &&
                ((!item.second.m_expectedName.empty() && peer.second.m_name == item.second.m_expectedName) ||
                 (peer.second.m_node.m_address == item.first.first && peer.second.m_node.m_port == item.first.second))) { connected = true; break; }
        }
        if (connected) { continue; }
        NetInterfaceLinkOptions options = {};
        options.m_version = 3;
        options.m_role = NI_LINK_NODE;
        options.m_name = m_name.c_str();
        options.m_tls.certificate = reinterpret_cast<const uint8_t*>(m_certificate.c_str());
        options.m_tls.certificateSize = static_cast<uint32_t>(m_certificate.size() + 1);
        options.m_tls.privateKey = reinterpret_cast<const uint8_t*>(m_privateKey.c_str());
        options.m_tls.privateKeySize = static_cast<uint32_t>(m_privateKey.size() + 1);
        options.m_tls.handshakeTimeoutMs = 5000;
        const uint64_t id = g_meshOutbound | ++m_lastOutbound;
        std::unique_ptr<NetGatewayNodeConnection> connection(new NetGatewayNodeConnection(*this, id,
            item.first.first, item.first.second, item.second.m_expectedName));
        const int32_t started = connection->connect(options);
        if (started == NI_OK) { m_outbound.insert(std::make_pair(id, std::move(connection))); }
        else
        {
            uint32_t random = 0;
            NetTransport::randomBytes(reinterpret_cast<uint8_t*>(&random), sizeof(random));
            item.second.m_retryAt = now + item.second.m_delay / 2 + random % item.second.m_delay;
            item.second.m_delay = (std::min)(item.second.m_delay * 2, 10000U);
        }
        break;
    }
}

void NetGatewayNode::pruneCandidates()
{
    for (auto candidate = m_candidates.begin(); candidate != m_candidates.end();)
    {
        bool retained = candidate->second.m_seed;
        for (auto member = m_members.begin(); !retained && member != m_members.end(); ++member)
        {
            retained = member->second.m_point.m_address == candidate->first.first &&
                member->second.m_point.m_port == candidate->first.second;
        }
        if (retained)
        {
            ++candidate;
        }
        else
        {
            candidate = m_candidates.erase(candidate);
        }
    }
}

void NetGatewayNode::expireMembers(uint64_t now)
{
    bool changed = false;
    for (auto member = m_members.begin(); member != m_members.end();)
    {
        if (now >= member->second.m_seen && now - member->second.m_seen >= 300000)
        {
            member = m_members.erase(member);
            changed = true;
        }
        else
        {
            ++member;
        }
    }
    if (changed)
    {
        m_cacheDirty = true;
        pruneCandidates();
    }
}