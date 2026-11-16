#include "NetGatewayNodeTestAccess.h"
#include "ProtoMessageManager/ProtoMessageManagerAPI.h"
#include <stdexcept>
#include <chrono>
#include <cstring>
#include <set>
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

/** 用被测池创建前置路由，不复制生产接纳或容量判断算法
@param [in,out] node 被测节点
@param [in] id 测试指定的唯一号
@param [in] value 前置状态，空参数创建默认路由
@return 池内借用记录，持续到删除此路由
*/
static NetGatewayNodeRoute& GatewayNodeTestRoute(NetGatewayNode& node, uint64_t id,
    const NetGatewayNodeRoute& value = NetGatewayNodeRoute())
{
    NetObjectReference<NetGatewayNodeRoute> record;
    if ((node.*netNodeTestMember(NetNodeTestRoutePool()))->create(record, value) != NI_OK)
    {
        throw std::runtime_error("test route pool allocation");
    }
    auto inserted = (node.*netNodeTestMember(NetNodeTestRoutes())).insert(std::make_pair(id, record));
    if (!inserted.second)
    {
        throw std::runtime_error("test duplicate route");
    }
    return *inserted.first->second;
}

/** 在被测节点的真实池中创建已拥有的密文，供关闭与背压前置状态使用
@param [in,out] node 正式节点，必须比返回租约晚销毁
@param [in] size 正文大小
@param [in] value 独立预期字节
@return 拥有的记录，准备失败直接报告测试失败
*/
static NetObjectReference<NetGatewayNodeRecord> NodeTestRecord(NetGatewayNode& node, uint32_t size, uint8_t value)
{
    std::vector<uint8_t> bytes(size, value);
    NetObjectReference<NetGatewayNodeRecord> record;
    if ((node.*netNodeTestMember(NetNodeTestRecordPool()))->copy(bytes.data(), size, record) != NI_OK)
    {
        throw std::runtime_error("node test record allocation");
    }
    return record;
}

int NetGatewayNodeLoadTest::check()
{
    uint32_t checks = 0;
    uint32_t failures = 0;
    const auto check = [&](bool pass, const char* name)
    {
        ++checks;
        if (!pass) { ++failures; std::printf("FAIL %s\n", name); }
    };
    NetGatewayNode node("node.test", "", "");
    (node.*netNodeTestMember(NetNodeTestBoot())) = 53;
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
    (node.*netNodeTestMember(NetNodeTestPeers())).insert(std::make_pair(2, remote));
    NetGatewayNodeSelection selected = {};
    check((node.*netNodeTestMember(NetNodeTestSelect()))(1, 101, 0, 100, selected) && selected.m_peer == 2 && selected.m_instance == 7 &&
        selected.m_boot == 79 && selected.m_window == 131072, "authenticated remote values");
    selected.m_peer = 999;
    check(!(node.*netNodeTestMember(NetNodeTestSelect()))(1, 102, 0, 100, selected) && selected.m_peer == 999, "product isolation and atomic no-result");
    check(!(node.*netNodeTestMember(NetNodeTestSelect()))(2, 101, 0, 100, selected), "no route back to requester");
    check(!(node.*netNodeTestMember(NetNodeTestSelect()))(1, 101, 7, 100, selected), "mesh exact instance cannot hop again");
    check((node.*netNodeTestMember(NetNodeTestSelect()))(1, 101, 0, 7099, selected), "snapshot just inside TTL");
    check(!(node.*netNodeTestMember(NetNodeTestSelect()))(1, 101, 0, 7100, selected), "expired snapshot excluded");
    check(!(node.*netNodeTestMember(NetNodeTestSelect()))(1, 101, 0, 99, selected), "future snapshot time excluded");
    (node.*netNodeTestMember(NetNodeTestPeers()))[2].m_closing = true;
    check(!(node.*netNodeTestMember(NetNodeTestSelect()))(1, 101, 0, 100, selected), "closing node excluded");
    (node.*netNodeTestMember(NetNodeTestPeers()))[2].m_closing = false;
    (node.*netNodeTestMember(NetNodeTestPeers()))[2].m_active = 512;
    check((node.*netNodeTestMember(NetNodeTestSelect()))(1, 101, 0, 100, selected), "node link has no fixed 512 channel gate");
    (node.*netNodeTestMember(NetNodeTestPeers()))[2].m_active = 0;
    (node.*netNodeTestMember(NetNodeTestPeers()))[2].m_remoteServices[0].m_capacity = 3;
    (node.*netNodeTestMember(NetNodeTestPeers()))[2].m_remoteServices[0].m_active = 1;
    NetGatewayNodeRoute reservation;
    reservation.m_service = 2;
    reservation.m_instance = 7;
    reservation.m_pending = true;
    GatewayNodeTestRoute(node, 1, reservation);
    GatewayNodeTestRoute(node, 2, reservation);
    check(!(node.*netNodeTestMember(NetNodeTestSelect()))(1, 101, 0, 100, selected), "pending admissions consume hard capacity");
    (node.*netNodeTestMember(NetNodeTestRoutes()))[1]->m_pending = (node.*netNodeTestMember(NetNodeTestRoutes()))[2]->m_pending = false;
    (node.*netNodeTestMember(NetNodeTestRoutes()))[1]->m_loadRevision = (node.*netNodeTestMember(NetNodeTestRoutes()))[2]->m_loadRevision = 11;
    check(!(node.*netNodeTestMember(NetNodeTestSelect()))(1, 101, 0, 100, selected), "acknowledged admissions retained before snapshot");
    (node.*netNodeTestMember(NetNodeTestPeers()))[2].m_snapshotRevision = 11;
    (node.*netNodeTestMember(NetNodeTestPeers()))[2].m_remoteServices[0].m_active = 3;
    check(!(node.*netNodeTestMember(NetNodeTestSelect()))(1, 101, 0, 100, selected), "new snapshot at hard capacity");
    (node.*netNodeTestMember(NetNodeTestPeers()))[2].m_remoteServices[0].m_active = 2;
    check((node.*netNodeTestMember(NetNodeTestSelect()))(1, 101, 0, 100, selected), "reflected admissions no longer double counted");
    (node.*netNodeTestMember(NetNodeTestPeers()))[2].m_snapshotRevision = 10;
    (node.*netNodeTestMember(NetNodeTestRoutes()))[1]->m_instance = (node.*netNodeTestMember(NetNodeTestRoutes()))[2]->m_instance = 8;
    check((node.*netNodeTestMember(NetNodeTestSelect()))(1, 101, 0, 100, selected), "reservations scoped to target instance");
    (node.*netNodeTestMember(NetNodeTestRoutes()))[1]->m_service = (node.*netNodeTestMember(NetNodeTestRoutes()))[2]->m_service = 3;
    (node.*netNodeTestMember(NetNodeTestRoutes()))[1]->m_instance = (node.*netNodeTestMember(NetNodeTestRoutes()))[2]->m_instance = 7;
    check((node.*netNodeTestMember(NetNodeTestSelect()))(1, 101, 0, 100, selected), "reservations scoped to node generation");
    (node.*netNodeTestMember(NetNodeTestRoutes())).clear();
    (node.*netNodeTestMember(NetNodeTestPeers()))[2].m_remoteServices[0] = service;
    (node.*netNodeTestMember(NetNodeTestPeers())).insert(std::make_pair(3, remote));
    uint32_t admitted[2] = {};
    for (uint64_t route = 1; route <= 12; ++route)
    {
        const bool found = (node.*netNodeTestMember(NetNodeTestSelect()))(1, 101, 0, 100, selected);
        check(found && (selected.m_peer == 2 || selected.m_peer == 3), "burst selects healthy target");
        if (!found) { break; }
        ++admitted[selected.m_peer - 2];
        reservation.m_service = selected.m_peer;
        reservation.m_instance = selected.m_instance;
        reservation.m_pending = false;
        reservation.m_loadRevision = 11;
        GatewayNodeTestRoute(node, route, reservation);
    }
    check(admitted[0] == 6 && admitted[1] == 6, "burst acknowledgements spread before snapshots");
    (node.*netNodeTestMember(NetNodeTestRoutes())).clear();
    (node.*netNodeTestMember(NetNodeTestPeers()))[2].m_remoteServices[0].m_active = 6;
    (node.*netNodeTestMember(NetNodeTestPeers()))[3].m_remoteServices[0].m_active = 1;
    check((node.*netNodeTestMember(NetNodeTestSelect()))(1, 101, 0, 100, selected) && selected.m_peer == 3, "prefer spare capacity");
    (node.*netNodeTestMember(NetNodeTestPeers()))[2].m_remoteServices[0].m_active = 0;
    (node.*netNodeTestMember(NetNodeTestPeers()))[3].m_remoteServices[0].m_active = 0;
    (node.*netNodeTestMember(NetNodeTestPeers()))[3].m_remoteServices[0].m_weight = 2;
    check((node.*netNodeTestMember(NetNodeTestSelect()))(1, 101, 0, 100, selected) && selected.m_peer == 3, "relative weight affects selection");
    (node.*netNodeTestMember(NetNodeTestPeers())).clear();
    NetGatewayNodePeer local;
    local.m_role = NI_LINK_SERVICE;
    local.m_serviceReady = true;
    local.m_product = 101;
    local.m_capacity = 8;
    local.m_window = 65536;
    (node.*netNodeTestMember(NetNodeTestPeers())).insert(std::make_pair(UINT64_C(0x8000000000000000) | 11, local));
    check((node.*netNodeTestMember(NetNodeTestSelect()))(1, 101, 11, 100, selected) && selected.m_instance == 11 && selected.m_boot == 53 &&
        selected.m_window == 65536, "local exact instance and boot");
    check(!(node.*netNodeTestMember(NetNodeTestSelect()))(1, 101, 12, 100, selected), "no substitution for requested instance");
    (node.*netNodeTestMember(NetNodeTestPeers()))[UINT64_C(0x8000000000000000) | 11].m_active = 8;
    check(!(node.*netNodeTestMember(NetNodeTestSelect()))(1, 101, 11, 100, selected), "target enforces own hard capacity");
    (node.*netNodeTestMember(NetNodeTestPeers()))[UINT64_C(0x8000000000000000) | 11].m_active = 0;
    remote.m_name = "node.alpha";
    remote.m_remoteServices.clear();
    (node.*netNodeTestMember(NetNodeTestPeers())).insert(std::make_pair(2, remote));
    (node.*netNodeTestMember(NetNodeTestSnapshotRevision())) = 10;
    ProtoMessage request;
    request.setKeyValue(NI_GATEWAY_TARGET_PRODUCT, Variant(static_cast<int32_t>(101)));
    request.setKeyValue(NI_GATEWAY_CHANNEL, Variant(UINT64_C(1)));
    request.setKeyValue(NI_GATEWAY_INSTANCE, Variant(UINT64_C(11)));
    request.setKeyValue(NI_GATEWAY_BOOT, Variant((node.*netNodeTestMember(NetNodeTestBoot()))));
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
    check((node.*netNodeTestMember(NetNodeTestOpen()))(2, frame, true) == NI_OK && (node.*netNodeTestMember(NetNodeTestRoutes())).size() == 1 &&
        (node.*netNodeTestMember(NetNodeTestPeers()))[UINT64_C(0x8000000000000000) | 11].m_active == 1, "real OPEN reserves target once");
    ProtoMessage response;
    Variant revision;
    check(!(node.*netNodeTestMember(NetNodeTestPeers()))[2].m_controls.empty() && response.from(std::string(reinterpret_cast<const char*>(
            (node.*netNodeTestMember(NetNodeTestPeers()))[2].m_controls.back()->m_payload.data()),
            (node.*netNodeTestMember(NetNodeTestPeers()))[2].m_controls.back()->m_frame.m_payloadSize)) &&
        response.getCode() == NI_OK && response.getKeyValue(NI_GATEWAY_LOAD_REVISION, revision) && revision.type() == Variant::UINT64 &&
        static_cast<uint64_t>(revision) == 11, "real mesh ACK carries future load revision");
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
        check((discovery.*netNodeTestMember(NetNodeTestRemember()))(point, true) == NI_OK, "authenticated address change remains admissible");
        check((discovery.*netNodeTestMember(NetNodeTestCandidates())).size() == 2, "old learned address must not consume a permanent candidate slot");
    }
    check(discovery.addSeed(point.m_address, point.m_port) == NI_OK, "learned endpoint can be promoted to configured seed");
    const uint32_t pinnedPort = point.m_port;
    ++point.m_port;
    ++point.m_boot;
    check((discovery.*netNodeTestMember(NetNodeTestRemember()))(point, true) == NI_OK && (discovery.*netNodeTestMember(NetNodeTestCandidates())).size() == 3,
        "moving a node retains configured seeds and only the current learned address");
    point.m_name = "node." + std::string(32, 'b');
    check((discovery.*netNodeTestMember(NetNodeTestRemember()))(point, true) == NI_OK && (discovery.*netNodeTestMember(NetNodeTestMembers())).size() == 1 && (discovery.*netNodeTestMember(NetNodeTestCandidates())).size() == 3,
        "new authenticated identity at the same address replaces old membership without candidate growth");
    // 固定白盒单调时钟，调用真实过期分支；不冒称执行过五分钟浸泡。
    (discovery.*netNodeTestMember(NetNodeTestMembers())).begin()->second->m_seen = 100;
    (discovery.*netNodeTestMember(NetNodeTestExpireMembers()))(300099);
    check((discovery.*netNodeTestMember(NetNodeTestMembers())).size() == 1 && (discovery.*netNodeTestMember(NetNodeTestCandidates())).size() == 3, "membership just inside expiry stays usable");
    (discovery.*netNodeTestMember(NetNodeTestExpireMembers()))(300100);
    check((discovery.*netNodeTestMember(NetNodeTestMembers())).empty() && (discovery.*netNodeTestMember(NetNodeTestCandidates())).size() == 2 &&
        (discovery.*netNodeTestMember(NetNodeTestCandidates())).count(std::make_pair(std::string("127.0.0.1"), 5203U)) == 1 &&
        (discovery.*netNodeTestMember(NetNodeTestCandidates())).count(std::make_pair(point.m_address, pinnedPort)) == 1,
        "expired learned membership releases its candidate while configured seeds survive");
    NetGatewayNode growing("node.growing", "", "");
    for (uint32_t index = 0; index < 512; ++index)
    {
        check(growing.addSeed("127.0.0.1", 10000 + index) == NI_OK, "candidate pool grows beyond 128");
    }
    check((growing.*netNodeTestMember(NetNodeTestCandidates())).size() == 512, "all distinct candidate records retained");
    NetGatewayNode tiny("node.tiny", "", "");
    LockFreeHashMapPoolConfig metadata;
    metadata.initialCapacity = 1;
    metadata.maximumCapacity = 2;
    check(tiny.configureDiscoveryPools(metadata) == NI_OK, "configure actual discovery pool");
    check(tiny.addSeed("127.0.0.1", 5203) == NI_OK && tiny.addSeed("127.0.0.1", 5204) == NI_OK &&
        tiny.addSeed("127.0.0.1", 5205) == NI_LIMIT_EXCEEDED, "only actual discovery storage controls admission");
    check(tiny.addSeed("127.0.0.1", 5203) == NI_OK, "full pool duplicate needs no extra slot");
    NetInterfaceDirectoryPoint learned;
    learned.m_name = "node.0123456789abcdef0123456789abcdef";
    learned.m_address = "127.0.0.1";
    learned.m_port = 5205;
    learned.m_capacity = 8;
    learned.m_boot = 123;
    check((tiny.*netNodeTestMember(NetNodeTestRemember()))(learned, true) == NI_LIMIT_EXCEEDED &&
        (tiny.*netNodeTestMember(NetNodeTestMembers())).empty(), "candidate pool failure cannot publish member");
    learned.m_port = 5203;
    check((tiny.*netNodeTestMember(NetNodeTestRemember()))(learned, true) == NI_OK, "existing candidate can publish after failed growth");
    learned.m_port = 5205;
    check((tiny.*netNodeTestMember(NetNodeTestRemember()))(learned, true) == NI_LIMIT_EXCEEDED &&
        (tiny.*netNodeTestMember(NetNodeTestMembers())).begin()->second->m_point.m_port == 5203,
        "failed replacement preserves verified endpoint");
    // 普通控制消息积压时，批量通道关闭不能终止仍健康的共享服务。
    NetGatewayNode retiring("node.closing", "", "");
    (retiring.*netNodeTestMember(NetNodeTestPeers()))[1].m_opened = 40;
    (retiring.*netNodeTestMember(NetNodeTestPeers()))[2].m_active = 40;
    NetInterfaceFrame normal = {};
    normal.m_kind = NI_EVENT;
    for (uint32_t index = 0; index < 32; ++index)
    {
        check((retiring.*netNodeTestMember(NetNodeTestQueueControl()))(2, normal, "") == NI_OK, "prepare ordinary controls before channel closure");
    }
    for (uint32_t index = 1; index <= 40; ++index)
    {
        NetGatewayNodeRoute route;
        route.m_client = 1;
        route.m_clientChannel = index;
        route.m_service = 2;
        route.m_serviceChannel = UINT64_C(0x8000000000000000) | index;
        GatewayNodeTestRoute(retiring, index, route);
        (retiring.*netNodeTestMember(NetNodeTestChannels()))[std::make_pair(1, route.m_clientChannel)] = index;
        (retiring.*netNodeTestMember(NetNodeTestChannels()))[std::make_pair(2, route.m_serviceChannel)] = index;
        (retiring.*netNodeTestMember(NetNodeTestRetire()))(index, 1, NI_TRUNCATED);
        check(!(retiring.*netNodeTestMember(NetNodeTestPeers()))[2].m_closing, "closing one channel must retain the shared service");
    }
    check((retiring.*netNodeTestMember(NetNodeTestRoutes())).empty() && (retiring.*netNodeTestMember(NetNodeTestChannels())).empty() && (retiring.*netNodeTestMember(NetNodeTestPeers()))[2].m_active == 0 &&
        (retiring.*netNodeTestMember(NetNodeTestPeers()))[1].m_opened == 0 && (retiring.*netNodeTestMember(NetNodeTestPeers()))[2].m_controls.size() == 72,
        "all channels release capacity and each close remains queued exactly once");
    check((retiring.*netNodeTestMember(NetNodeTestQueueControl()))(2, normal, "") == NI_OK,
        "normal control remains admissible after the old 32 item boundary");
    NetInterfaceFrame closing = {};
    closing.m_kind = NI_EVENT;
    closing.m_messageType = NI_GATEWAY_CHANNEL_CLOSE;
    check((retiring.*netNodeTestMember(NetNodeTestQueueControl()))(2, closing, std::string(129, 'x')) == NI_OK,
        "close storage has no separate 128 byte gate");
    auto& controls = (retiring.*netNodeTestMember(NetNodeTestPeers()))[2].m_controls;
    controls.clear();
    for (uint32_t round = 0; round < 2; ++round)
    {
        for (uint32_t index = 0; index < 8192; ++index)
        {
            const std::string bytes(index % 128 == 0 ? 16384 : 6, static_cast<char>(index % 251));
            const NetInterfaceFrame& metadata = index % 2 == 0 ? normal : closing;
            check((retiring.*netNodeTestMember(NetNodeTestQueueControl()))(2, metadata, bytes) == NI_OK,
                "control pool grows past both former count and byte gates");
        }
        check(controls.size() == 8192, "all controls retained");
        for (uint32_t index = 0; index < 8192 && !controls.empty(); ++index)
        {
            const std::string expected(index % 128 == 0 ? 16384 : 6, static_cast<char>(index % 251));
            const NetGatewayNodeControl& pending = *controls.front();
            check(pending.m_frame.m_payloadSize == expected.size() && pending.m_frame.m_payload == nullptr &&
                pending.m_frame.m_messageType == (index % 2 == 0 ? normal.m_messageType : closing.m_messageType) &&
                std::memcmp(pending.m_payload.data(), expected.data(), expected.size()) == 0,
                "control FIFO owns exact original payload");
            controls.pop_front();
        }
        check(controls.empty(), "control pool drain and reuse");
    }
    // 一槽显式物理池：OPEN需要两条输出时必须全有或全无，不能先改路由再发现第二槽耗尽。
    NetGatewayNode atomicOpen("node.atomic", "", "");
    NetBufferPoolConfig controlBodies;
    LockFreeHashMapPoolConfig controlRecords;
    controlRecords.initialCapacity = 1;
    controlRecords.maximumCapacity = 1;
    check(atomicOpen.configureControlPools(controlBodies, controlRecords) == NI_OK, "configure exact control pool fixture");
    controlRecords.initialCapacity = 2;
    check(atomicOpen.configureControlPools(controlBodies, controlRecords) == NI_INVALID_ARGUMENT,
        "invalid control config preserves previous pool");
    (atomicOpen.*netNodeTestMember(NetNodeTestBoot())) = 53;
    (atomicOpen.*netNodeTestMember(NetNodeTestPeers()))[2] = remote;
    (atomicOpen.*netNodeTestMember(NetNodeTestPeers()))[UINT64_C(0x8000000000000000) | 11] = local;
    check((atomicOpen.*netNodeTestMember(NetNodeTestOpen()))(2, frame, true) == NI_LIMIT_EXCEEDED &&
        (atomicOpen.*netNodeTestMember(NetNodeTestRoutes())).empty() &&
        (atomicOpen.*netNodeTestMember(NetNodeTestChannels())).empty() &&
        (atomicOpen.*netNodeTestMember(NetNodeTestPeers()))[2].m_controls.empty() &&
        (atomicOpen.*netNodeTestMember(NetNodeTestPeers()))[2].m_highestChannel == 0 &&
        (atomicOpen.*netNodeTestMember(NetNodeTestPeers()))[UINT64_C(0x8000000000000000) | 11].m_active == 0,
        "failed two-output OPEN rolls back actual storage and all route state");
    check(atomicOpen.configureControlPools(controlBodies, controlRecords) == NI_INVALID_STATE,
        "control pool cannot be replaced after connections exist");
    for (uint32_t round = 0; round < 4; ++round)
    {
        auto& queue = (atomicOpen.*netNodeTestMember(NetNodeTestPeers()))[2].m_controls;
        check((atomicOpen.*netNodeTestMember(NetNodeTestQueueControl()))(2, normal, "123456") == NI_OK &&
            (atomicOpen.*netNodeTestMember(NetNodeTestQueueControl()))(2, closing, "x") == NI_LIMIT_EXCEEDED &&
            queue.size() == 1 && queue.front()->m_frame.m_payloadSize == 6,
            "actual full control pool rejects without changing existing data");
        queue.clear();
    }
    // 接收入口真实处理满队列，必须保留路由和未消费数据，不能以容量不足关闭合法连接。
    NetGatewayNode bounded("node.bounded", "", "");
    NetBufferPoolConfig bodies;
    LockFreeHashMapPoolConfig records = NetGatewayNodeRecordPool::defaultRecords();
    records.initialCapacity = 0;
    records.maximumCapacity = 64;
    check(bounded.configureRecordPools(bodies, records) == NI_OK, "node explicit physical record pool");
    records.initialCapacity = 65;
    check(bounded.configureRecordPools(bodies, records) == NI_INVALID_ARGUMENT, "node invalid pool rollback");
    NetGatewayNodeRoute& buffered = GatewayNodeTestRoute(bounded, 1);
    buffered.m_client = 1;
    buffered.m_service = 2;
    buffered.m_clientChannel = 11;
    buffered.m_serviceChannel = 22;
    buffered.m_product = 101;
    (bounded.*netNodeTestMember(NetNodeTestChannels()))[std::make_pair(UINT64_C(1), UINT64_C(11))] = 1;
    (bounded.*netNodeTestMember(NetNodeTestChannels()))[std::make_pair(UINT64_C(2), UINT64_C(22))] = 1;
    NetInterfaceTunnelRecord input = {};
    input.m_kind = NI_TUNNEL_BYTES;
    input.m_product = 101;
    input.m_data = reinterpret_cast<const uint8_t*>("123456");
    input.m_size = 6;
    for (uint32_t direction = 0; direction < 2; ++direction)
    {
        auto& queue = direction == 0 ? buffered.m_forwardQueue : buffered.m_reverseQueue;
        auto& credit = direction == 0 ? buffered.m_clientWindow : buffered.m_serviceWindow;
        const uint64_t source = direction == 0 ? 1 : 2;
        input.m_channel = direction == 0 ? 11 : 22;
        for (uint32_t index = 0; index < 64; ++index) { queue.push_back(NodeTestRecord(bounded, 6, 9)); }
        (bounded.*netNodeTestMember(NetNodeTestRecordBytes())) = 384;
        const uint32_t available = credit.available();
        check(!(bounded.*netNodeTestMember(NetNodeTestOnLinkRecord()))(source, input) &&
            (bounded.*netNodeTestMember(NetNodeTestRoutes())).size() == 1 && queue.size() == 64 &&
            credit.available() == available && (bounded.*netNodeTestMember(NetNodeTestRecordBytes())) == 384,
            "full route queue retains record without closing or debiting credit");
        queue.pop_front();
        (bounded.*netNodeTestMember(NetNodeTestRecordBytes())) -= 6;
        check((bounded.*netNodeTestMember(NetNodeTestOnLinkRecord()))(source, input) && queue.size() == 64 &&
            credit.available() == available - 6 && queue.back()->m_size == 6 && std::memcmp(queue.back()->m_bytes.data(), input.m_data, 6) == 0,
            "same head retries exactly once after capacity release");
        queue.clear();
        (bounded.*netNodeTestMember(NetNodeTestRecordBytes())) = 0;
    }

    // 默认配置在同一方向积累2048条，不依赖远端业务回复；正文以独立模式核对。
    NetGatewayNode burst("node.burst", "", "");
    NetGatewayNodeRoute& burstRoute = GatewayNodeTestRoute(burst, 1);
    burstRoute.m_client = 1; burstRoute.m_service = 2;
    burstRoute.m_clientChannel = 11; burstRoute.m_serviceChannel = 22; burstRoute.m_product = 101;
    (burst.*netNodeTestMember(NetNodeTestChannels()))[std::make_pair(UINT64_C(1), UINT64_C(11))] = 1;
    burstRoute.m_forwardQueue.push_back(NodeTestRecord(burst, 6, 3));
    (burst.*netNodeTestMember(NetNodeTestRecordBytes())) = 6;
    input.m_channel = 11;
    const uint32_t beforeCredit = burstRoute.m_clientWindow.available();
    for (uint32_t index = 0; index < 2048; ++index)
    {
        check((burst.*netNodeTestMember(NetNodeTestOnLinkRecord()))(1, input), "node default record pool retained old 64 gate");
    }
    check(burstRoute.m_forwardQueue.size() == 2049 && burstRoute.m_clientWindow.available() == beforeCredit - 2048 * 6 &&
        (burst.*netNodeTestMember(NetNodeTestRecordBytes())) == 2049 * 6, "node default burst exact credit and storage");
    burstRoute.m_forwardQueue.pop_front();
    (burst.*netNodeTestMember(NetNodeTestRecordBytes())) -= 6;
    for (const auto& item : burstRoute.m_forwardQueue)
    {
        check(item->m_size == 6 && std::memcmp(item->m_bytes.data(), "123456", 6) == 0, "node default burst payload ownership");
    }
    // 正文池与记录池独立耗尽，释放一份后原记录可重试，不能提前扣信用。
    NetGatewayNode bodyBounded("node.body-pool", "", "");
    bodies.m_classes[0].initialCapacity = 0; bodies.m_classes[0].maximumCapacity = 2;
    records = NetGatewayNodeRecordPool::defaultRecords();
    check(bodyBounded.configureRecordPools(bodies, records) == NI_OK, "node body pool configuration");
    NetGatewayNodeRoute& bodyRoute = GatewayNodeTestRoute(bodyBounded, 1);
    bodyRoute.m_client = 1; bodyRoute.m_service = 2;
    bodyRoute.m_clientChannel = 11; bodyRoute.m_serviceChannel = 22; bodyRoute.m_product = 101;
    (bodyBounded.*netNodeTestMember(NetNodeTestChannels()))[std::make_pair(UINT64_C(1), UINT64_C(11))] = 1;
    for (uint32_t index = 0; index < 2; ++index) { bodyRoute.m_forwardQueue.push_back(NodeTestRecord(bodyBounded, 6, 9)); }
    (bodyBounded.*netNodeTestMember(NetNodeTestRecordBytes())) = 12;
    const uint32_t bodyCredit = bodyRoute.m_clientWindow.available();
    check(!(bodyBounded.*netNodeTestMember(NetNodeTestOnLinkRecord()))(1, input) && bodyRoute.m_clientWindow.available() == bodyCredit,
        "node actual body pool full preserves input and credit");
    bodyRoute.m_forwardQueue.pop_front();
    (bodyBounded.*netNodeTestMember(NetNodeTestRecordBytes())) -= 6;
    check((bodyBounded.*netNodeTestMember(NetNodeTestOnLinkRecord()))(1, input) && bodyRoute.m_forwardQueue.size() == 2 &&
        bodyRoute.m_clientWindow.available() == bodyCredit - 6, "node body slot reclaimed and input retried once");
    check(bodyBounded.configureRecordPools(bodies, records) == NI_INVALID_STATE, "node active route cannot replace its pool");

    // 把实际正文池配足，十二个合法信用窗口合计缓存9MiB，验证没有旧的8MiB额外门槛。
    NetGatewayNode largePool("node.large-pool", "", "");
    bodies = NetBufferPoolConfig();
    bodies.m_classes[2].maximumCapacity = 768;
    check(largePool.configureRecordPools(bodies, records) == NI_OK, "node large body pool configuration");
    std::vector<uint8_t> largeBody(16384, 0x53);
    for (uint64_t routeId = 1; routeId <= 12; ++routeId)
    {
        NetGatewayNodeRoute& route = GatewayNodeTestRoute(largePool, routeId);
        route.m_client = routeId; route.m_service = 100;
        route.m_clientChannel = routeId; route.m_serviceChannel = 100 + routeId; route.m_product = 101;
        check(route.m_clientWindow.configure(1024 * 1024) == NI_OK, "node test peer credit window");
        (largePool.*netNodeTestMember(NetNodeTestChannels()))[std::make_pair(routeId, routeId)] = routeId;
        route.m_forwardQueue.push_back(NodeTestRecord(largePool, 1, 9));
        ++(largePool.*netNodeTestMember(NetNodeTestRecordBytes()));
        input.m_channel = routeId; input.m_data = largeBody.data(); input.m_size = static_cast<uint32_t>(largeBody.size());
        for (uint32_t index = 0; index < 48; ++index)
        {
            check((largePool.*netNodeTestMember(NetNodeTestOnLinkRecord()))(routeId, input), "node retained old total byte gate");
        }
        check(route.m_forwardQueue.size() == 49 && route.m_clientWindow.available() == 256 * 1024,
            "node large pooled forwarding exact credit");
    }
    check((largePool.*netNodeTestMember(NetNodeTestRecordBytes())) == 9 * 1024 * 1024 + 12,
        "node accepts above eight MiB when its actual body pool permits");

    // 4096条已建立且无积压的通道不应在每轮转发／OPEN期限检查中逐条做无用工作。
    // 此处只测管理状态的CPU耗时；真实网络吞吐和故障迁移由NetWorkTest多进程用例验证。
    NetGatewayNode idle("node.idle", "", "");
    (idle.*netNodeTestMember(NetNodeTestSnapshot())) = "fixed test snapshot";
    (idle.*netNodeTestMember(NetNodeTestSnapshotAt())) = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
    (idle.*netNodeTestMember(NetNodeTestMeshDirty())) = false;
    (idle.*netNodeTestMember(NetNodeTestLastOutbound())) = UINT64_C(0x4000000000000000) - 1;
    NetGatewayNodeRoute established;
    for (uint64_t id = 1; id <= 4096; ++id) { GatewayNodeTestRoute(idle, id, established); }
#ifdef _WIN32
    LARGE_INTEGER idleBegin = {};
    LARGE_INTEGER frequency = {};
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&idleBegin);
#else
    const auto idleBegin = std::chrono::steady_clock::now();
#endif
    for (uint32_t step = 0; step < 5000; ++step) { (idle.*netNodeTestMember(NetNodeTestFlushRecords()))(); (idle.*netNodeTestMember(NetNodeTestPollMesh()))(); }
#ifdef _WIN32
    LARGE_INTEGER idleEnd = {};
    QueryPerformanceCounter(&idleEnd);
    const long long idleMicros = (idleEnd.QuadPart - idleBegin.QuadPart) * 1000000 / frequency.QuadPart;
#else
    const long long idleMicros = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - idleBegin).count();
#endif
    check((idle.*netNodeTestMember(NetNodeTestRoutes())).size() == 4096 && (idle.*netNodeTestMember(NetNodeTestRecordBytes())) == 0 && (idle.*netNodeTestMember(NetNodeTestPendingOpens())).empty(),
        "idle polls retain established channels and accounting");
    for (uint64_t id = 4097; id <= 4100; ++id)
    {
        NetGatewayNodeRoute pending;
        pending.m_pending = true;
        pending.m_service = 2;
        pending.m_remoteRequest = id;
        pending.m_deadline = id == 4100 ? UINT64_MAX : 1;
        GatewayNodeTestRoute(idle, id, pending);
        (idle.*netNodeTestMember(NetNodeTestPendingOpens())).insert(std::make_pair(std::make_pair(pending.m_service, id), id));
    }
    (idle.*netNodeTestMember(NetNodeTestPollMesh()))();
    check((idle.*netNodeTestMember(NetNodeTestRoutes())).size() == 4097 && (idle.*netNodeTestMember(NetNodeTestPendingOpens())).size() == 1 &&
        (idle.*netNodeTestMember(NetNodeTestRoutes())).count(4100) == 1 && (idle.*netNodeTestMember(NetNodeTestPendingOpens())).begin()->second == 4100,
        "adjacent expired OPENs retire once; future and established routes remain");
    (idle.*netNodeTestMember(NetNodeTestRoutes()))[4100]->m_deadline = 1;
    (idle.*netNodeTestMember(NetNodeTestSnapshotAt())) = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
    (idle.*netNodeTestMember(NetNodeTestMeshDirty())) = false;
    (idle.*netNodeTestMember(NetNodeTestPollMesh()))();
    check((idle.*netNodeTestMember(NetNodeTestRoutes())).size() == 4096 && (idle.*netNodeTestMember(NetNodeTestPendingOpens())).empty(), "last pending OPEN releases its index");
    // 有积压时仍必须推进真实发送错误和清理，不能把快速路径写成无条件跳过。
    NetGatewayNodeRoute broken;
    broken.m_client = 1;
    broken.m_service = 2;
    broken.m_forwardQueue.push_back(NodeTestRecord(idle, 16, 7));
    broken.m_reverseQueue.push_back(NodeTestRecord(idle, 8, 9));
    GatewayNodeTestRoute(idle, 4101, broken);
    (idle.*netNodeTestMember(NetNodeTestRecordBytes())) = 24;
    (idle.*netNodeTestMember(NetNodeTestFlushRecords()))();
    check((idle.*netNodeTestMember(NetNodeTestRoutes())).size() == 4096 && (idle.*netNodeTestMember(NetNodeTestRoutes())).count(4101) == 0 && (idle.*netNodeTestMember(NetNodeTestRecordBytes())) == 0,
        "nonempty failed route is retired and both directions release bytes");

#if defined(_MSC_VER) && defined(_DEBUG)
    // 原关闭路径分配临时vector并复制路由；内存紧张时抛出会使终态回调留下孤儿路由。
    NetGatewayNode exhausted("node.oom-close", "", "");
    (exhausted.*netNodeTestMember(NetNodeTestPeers()))[1].m_opened = 2;
    (exhausted.*netNodeTestMember(NetNodeTestPeers()))[2].m_active = 2;
    for (uint64_t id = 1; id <= 2; ++id)
    {
        NetGatewayNodeRoute route;
        route.m_client = 1;
        route.m_clientChannel = id;
        route.m_service = 2;
        route.m_serviceChannel = UINT64_C(0x8000000000000000) | id;
        route.m_pending = id == 2;
        route.m_remoteRequest = id;
        route.m_forwardQueue.push_back(NodeTestRecord(exhausted, 16, 1));
        route.m_reverseQueue.push_back(NodeTestRecord(exhausted, 8, 2));
        GatewayNodeTestRoute(exhausted, id, route);
        (exhausted.*netNodeTestMember(NetNodeTestChannels()))[std::make_pair(1, route.m_clientChannel)] = id;
        (exhausted.*netNodeTestMember(NetNodeTestChannels()))[std::make_pair(2, route.m_serviceChannel)] = id;
        if (route.m_pending) { (exhausted.*netNodeTestMember(NetNodeTestPendingOpens()))[std::make_pair(2, id)] = id; }
        (exhausted.*netNodeTestMember(NetNodeTestRecordBytes())) += 24;
    }
    bool closeThrew = false;
    g_nodeAllocationBudget = 0;
    _CRT_ALLOC_HOOK previousHook = _CrtSetAllocHook(GatewayNodeAllocationHook);
    try { (exhausted.*netNodeTestMember(NetNodeTestOnLinkClosed()))(1, NI_TRUNCATED); }
    catch (...) { closeThrew = true; }
    _CrtSetAllocHook(previousHook);
    g_nodeAllocationBudget = -1;
    check(!closeThrew && (exhausted.*netNodeTestMember(NetNodeTestRoutes())).empty() && (exhausted.*netNodeTestMember(NetNodeTestChannels())).empty() &&
        (exhausted.*netNodeTestMember(NetNodeTestPendingOpens())).empty() && (exhausted.*netNodeTestMember(NetNodeTestRecordBytes())) == 0 && (exhausted.*netNodeTestMember(NetNodeTestPeers())).count(1) == 0 &&
        (exhausted.*netNodeTestMember(NetNodeTestPeers()))[2].m_active == 0, "allocation failure cannot interrupt route teardown");
    check((exhausted.*netNodeTestMember(NetNodeTestPeers()))[2].m_closing, "unavailable close notification must terminate its recipient safely");

    uint32_t failedOpenAllocations = 0;
    uint32_t successfulOpenPaths = 0;
    for (uint32_t remoteTarget = 0; remoteTarget < 2; ++remoteTarget)
    for (int32_t budget = 0; budget < 1024; ++budget)
    {
        NetGatewayNode opening("node.oom-open", "", "");
        (opening.*netNodeTestMember(NetNodeTestBoot())) = 19;
        (opening.*netNodeTestMember(NetNodeTestLastRoute())) = 1;
        (opening.*netNodeTestMember(NetNodeTestPeers()))[1].m_role = NI_LINK_CLIENT;
        (opening.*netNodeTestMember(NetNodeTestPeers()))[3].m_opened = 1;
        const uint64_t target = remoteTarget == 0 ? (UINT64_C(0x8000000000000000) | 11) : 2;
        NetGatewayNodePeer& destination = (opening.*netNodeTestMember(NetNodeTestPeers()))[target];
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
            destination.m_snapshotReceived = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
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
        prior.m_serviceChannel = UINT64_C(0x8000000000000000) | 1;
        GatewayNodeTestRoute(opening, 1, prior);
        (opening.*netNodeTestMember(NetNodeTestChannels()))[std::make_pair(3, prior.m_clientChannel)] = 1;
        (opening.*netNodeTestMember(NetNodeTestChannels()))[std::make_pair(target, prior.m_serviceChannel)] = 1;
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
        try { (opening.*netNodeTestMember(NetNodeTestOpen()))(1, openingFrame, false); }
        catch (...) { }
        const bool rejected = g_nodeAllocationBudget == 0;
        _CrtSetAllocHook(previousHook);
        g_nodeAllocationBudget = -1;
        if (rejected) { ++failedOpenAllocations; }
        bool indexed = (opening.*netNodeTestMember(NetNodeTestChannels())).size() == (opening.*netNodeTestMember(NetNodeTestRoutes())).size() * 2;
        for (const auto& route : (opening.*netNodeTestMember(NetNodeTestRoutes())))
        {
            indexed = indexed && (opening.*netNodeTestMember(NetNodeTestChannels())).count(std::make_pair(route.second->m_client, route.second->m_clientChannel)) == 1 &&
                (opening.*netNodeTestMember(NetNodeTestChannels())).count(std::make_pair(route.second->m_service, route.second->m_serviceChannel)) == 1;
        }
        check(indexed, "partial OPEN allocation must not publish orphan indexes");
        (opening.*netNodeTestMember(NetNodeTestOnLinkClosed()))(1, NI_NO_MEMORY);
        check((opening.*netNodeTestMember(NetNodeTestRoutes())).size() == 1 && (opening.*netNodeTestMember(NetNodeTestRoutes())).count(1) == 1 && (opening.*netNodeTestMember(NetNodeTestChannels())).size() == 2 &&
            (opening.*netNodeTestMember(NetNodeTestPeers()))[target].m_active == 1 && (opening.*netNodeTestMember(NetNodeTestPeers()))[3].m_opened == 1,
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

void NetGatewayNodeLoadTest::run()
{
    if (check() != 0)
    {
        throw std::runtime_error("gateway node load boundary failed");
    }
}