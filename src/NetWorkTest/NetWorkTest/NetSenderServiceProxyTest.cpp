#include "NetServiceTestAccess.h"
#include <LibuvTcp/LibuvTcpAPI.h>
#include "NetWorkConcurrencyTestCases.h"
#include "NetWorkTestAccess.h"
#include "NetSenderTestAccess.h"
#include "NetTestTlsCertificates.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "NetWorkTestCases.h"
#include <stdexcept>
#include <cstdio>
#include <cstring>
#include <chrono>
#include <algorithm>
#include <cstdlib>
#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif

#if defined(_WIN32) && defined(_DEBUG)
#include <crtdbg.h>
static int32_t s_proxyRejectAfter = -1;
static uint32_t s_proxyAllocations = 0;

/** 隔离main对普通堆逐位置拒绝分配，或统计热路径分配
@param [in] operation CRT分配操作；其他参数沿用CRT原型
@param [in] pointer CRT传入的堆块，只在钩子返回前借用；本用例未使用时忽略
@param [in] size CRT请求的字节数，是否参与注入判定由本用例决定
@param [in] block CRT堆块类别，只对普通业务堆块注入
@param [in] request CRT分配序号，本用例不使用
@param [in] filename CRT传入的来源文件名，本用例不读取
@param [in] line 来源代码行号，本用例不使用
@return 是否允许本次分配
*/
static int __cdecl ProxyMemoryHook(int operation, void* pointer, size_t size, int block, long request, const unsigned char* filename, int line)
{
    (void)pointer;
    (void)size;
    (void)request;
    (void)filename;
    (void)line;
    if (block == _NORMAL_BLOCK && (operation == _HOOK_ALLOC || operation == _HOOK_REALLOC))
    {
        ++s_proxyAllocations;
        if (s_proxyRejectAfter == 0) { return 0; }
        if (s_proxyRejectAfter > 0) { --s_proxyRejectAfter; }
    }
    return 1;
}
#endif

#include "NetSenderTestAccess.h"
#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif

bool NetSenderServiceProxyTest::run(bool memoryAudit)
{
    (void)memoryAudit;
    uint32_t checks = 0;
#define PROXY_CHECK(value) do { ++checks; if (!(value)) { std::printf("Proxy check line=%d FAIL\n", __LINE__); return false; } } while (0)
    NetSenderLocalInbox inbox;
    NetSenderService service(inbox);
    // 白盒只使编号分配器就绪；实际TLS、IPC与转发另由真实进程用例覆盖。
    ((service).*netServiceTestMember(NetServiceTestServiceState()))->m_endpoint = 1;
    NetSenderServiceProxy proxy(service);
    // 只在本边界用例中配置32个真实记录槽及8个大正文槽。
    NetBufferPoolConfig bodies;
    bodies.m_classes[4].maximumCapacity = 8;
    LockFreeHashMapPoolConfig records;
    records.initialCapacity = records.maximumCapacity = 32;
    records.lookupCacheCapacity = 0;
    PROXY_CHECK(proxy.configurePools(bodies, records) == NI_OK);
    records.initialCapacity = 33;
    PROXY_CHECK(proxy.configurePools(bodies, records) == NI_INVALID_ARGUMENT);
    records.initialCapacity = 32;
    NetSenderServiceProxyState& state = *((proxy).*netServiceTestMember(NetServiceTestProxyState()));
    NetInterfaceOrigin origin = {};
    origin.m_endpoint = 91;
    origin.m_platform = 1;
    origin.m_device[0] = 37;
    uint8_t body[4] = {0, 255, 13, 7};
    uint8_t input[20] = {};
    uint32_t required = 0;
    PROXY_CHECK(NetInterfaceRpc::encode(NI_REQUEST, 5000, 0, body, 4, input, sizeof(input), required) == NI_OK);
    NetInterfaceFrame root = {};
    root.m_kind = NI_REQUEST; root.m_productId = 9; root.m_messageType = 700;
    root.m_payload = input; root.m_payloadSize = sizeof(input);
    NetInterfaceFrame output = {};
    for (uint32_t index = 0; index < 32; ++index)
    {
        root.m_requestId = 100 + index;
        PROXY_CHECK(proxy.accept(root, origin) && state.m_count == index + 1 && state.m_bytes == (index + 1) * 4);
    }
    root.m_requestId = 200;
    PROXY_CHECK(!proxy.accept(root, origin) && proxy.valid() && state.m_count == 32 && state.m_bytes == 128);
    PROXY_CHECK(((service).*netServiceTestMember(NetServiceTestServiceState()))->m_lastAllocated == 0);
    // 中间槽位释放后必须追加至尾，不能按槽位下标打乱根请求顺序。
    state.erase(3);
    PROXY_CHECK(proxy.accept(root, origin) && state.m_last == 3 && state.m_first == 0 && state.m_count == 32);
    uint64_t previous = 0;
    uint32_t seen = 0;
    for (uint32_t index = state.m_first; index != UINT32_MAX; index = state.call(index).m_next)
    {
        NetSenderProxyCall& call = state.call(index);
        PROXY_CHECK(state.prepare(index, call.m_deadline - 5000, output));
        NetInterfaceRpcView rpc = {};
        PROXY_CHECK(NetInterfaceRpc::parse(output, rpc) == NI_OK && rpc.m_value == 5000 && rpc.m_parent == call.m_root &&
            rpc.m_size == 4 && std::memcmp(rpc.m_data, body, 4) == 0 && output.m_requestId > previous);
        previous = output.m_requestId;
        PROXY_CHECK(!state.finish(index, NI_OK) && call.m_phase == NET_SERVICE_PROXY_WAITING && call.m_size == 0 && call.m_wire.data() == nullptr);
        ++seen;
    }
    PROXY_CHECK(seen == 32 && state.m_bytes == 0);
    proxy.reset();
    PROXY_CHECK(proxy.valid() && state.m_count == 0 && state.m_first == UINT32_MAX && state.m_last == UINT32_MAX);
    root.m_requestId = 300;
    PROXY_CHECK(proxy.accept(root, origin));
    NetSenderProxyCall& first = state.call(state.m_first);
    const uint64_t now = first.m_deadline - 5000;
    uint64_t lastAttempt = 0;
    bool valid = true;
#if defined(_WIN32) && defined(_DEBUG)
    s_proxyAllocations = 0;
    _CRT_ALLOC_HOOK oldHook = memoryAudit ? _CrtSetAllocHook(ProxyMemoryHook) : nullptr;
#endif
    for (uint32_t index = 0; index < 10000; ++index)
    {
        valid = valid && state.prepare(state.m_first, now, output) && output.m_requestId > lastAttempt;
        lastAttempt = output.m_requestId;
        valid = valid && state.finish(state.m_first, NI_LIMIT_EXCEEDED) && first.m_child == 0 && first.m_size == 4;
    }
#if defined(_WIN32) && defined(_DEBUG)
    if (memoryAudit) { _CrtSetAllocHook(oldHook); PROXY_CHECK(s_proxyAllocations == 0); }
#endif
    PROXY_CHECK(valid && state.m_bytes == 4 && state.m_count == 1 && proxy.valid());
    PROXY_CHECK(state.prepare(state.m_first, now, output));
    const uint64_t child = output.m_requestId;
    PROXY_CHECK(!state.finish(state.m_first, NI_OK));
    NetInterfaceFrame reply = root;
    reply.m_kind = NI_RESPONSE;
    reply.m_requestId = child;
    uint8_t response[20] = {};
    PROXY_CHECK(NetInterfaceRpc::encode(NI_RESPONSE, NI_RPC_SUCCESS, 0, body, 4, response, 20, required) == NI_OK);
    reply.m_payload = response;
    PROXY_CHECK(proxy.accept(reply, origin) && first.m_phase == NET_SERVICE_PROXY_REPLY && state.m_bytes == 4);
    PROXY_CHECK(state.prepare(state.m_first, now + 100, output) && output.m_kind == NI_RESPONSE && output.m_requestId == root.m_requestId);
    NetInterfaceRpcView decoded = {};
    PROXY_CHECK(NetInterfaceRpc::parse(output, decoded) == NI_OK && decoded.m_parent == 0 && decoded.m_size == 4 &&
        std::memcmp(decoded.m_data, body, 4) == 0 && !state.finish(state.m_first, NI_LIMIT_EXCEEDED));
    PROXY_CHECK(state.prepare(state.m_first, now + 200, output) && !state.finish(state.m_first, NI_OK) && state.m_count == 0 && state.m_bytes == 0);
    PROXY_CHECK(proxy.accept(reply, origin) && state.m_count == 0 && proxy.valid());
    // 请求或响应/取消的来源、产品、命令不匹配，一次性清空本代，不能留下私密正文。
    for (uint32_t mode = 0; mode < 12; ++mode)
    {
        proxy.reset(); root.m_requestId = 400 + mode;
        PROXY_CHECK(proxy.accept(root, origin));
        NetInterfaceFrame changed = root;
        NetInterfaceOrigin changedOrigin = origin;
        if (mode < 6)
        {
            changed.m_kind = NI_CANCEL; changed.m_payload = nullptr; changed.m_payloadSize = 0;
        }
        else
        {
            PROXY_CHECK(state.prepare(state.m_first, state.call(state.m_first).m_deadline - 5000, output));
            changed = reply; changed.m_requestId = output.m_requestId;
            PROXY_CHECK(!state.finish(state.m_first, NI_OK));
        }
        switch (mode % 6)
        {
        case 0: ++changed.m_productId; break;
        case 1: ++changed.m_messageType; break;
        case 2: ++changedOrigin.m_endpoint; break;
        case 3: ++changedOrigin.m_platform; break;
        case 4: ++changedOrigin.m_device[3]; break;
        default: changed.m_payload = nullptr; changed.m_payloadSize = 1; break;
        }
        PROXY_CHECK(proxy.accept(changed, changedOrigin) && !proxy.valid() && state.m_count == 0 && state.m_bytes == 0);
    }
    // 取消未交付请求不发子取消；已经交付则有限等待，重复取消不延长期限。
    for (uint32_t mode = 0; mode < 5; ++mode)
    {
        proxy.reset(); root.m_requestId = 500 + mode;
        PROXY_CHECK(proxy.accept(root, origin));
        const uint32_t slot = state.m_first;
        NetSenderProxyCall& call = state.call(slot);
        if (mode != 0)
        {
            PROXY_CHECK(state.prepare(slot, call.m_deadline - 5000, output) && !state.finish(slot, NI_OK));
        }
        NetInterfaceFrame cancel = root; cancel.m_kind = NI_CANCEL; cancel.m_payload = nullptr; cancel.m_payloadSize = 0;
        PROXY_CHECK(proxy.accept(cancel, origin));
        if (mode == 0) { PROXY_CHECK(state.m_count == 0); continue; }
        const uint64_t until = call.m_cancelUntil;
        PROXY_CHECK(proxy.accept(cancel, origin) && call.m_cancelUntil == until && call.m_phase == NET_SERVICE_PROXY_CANCEL);
        if (mode == 4)
        {
            reply.m_requestId = call.m_child;
            PROXY_CHECK(proxy.accept(reply, origin) && state.m_count == 0);
        }
        else if (mode == 3)
        {
            PROXY_CHECK(!state.prepare(slot, until, output) && state.m_count == 0);
        }
        else
        {
            PROXY_CHECK(state.prepare(slot, until - 1, output) && output.m_kind == NI_CANCEL && output.m_requestId == call.m_child);
            PROXY_CHECK(!state.finish(slot, mode == 1 ? NI_OK : NI_NOT_FOUND) && state.m_count == 0);
        }
    }
    for (uint32_t phase = 0; phase < 3; ++phase)
    {
        proxy.reset(); root.m_requestId = 600 + phase;
        PROXY_CHECK(proxy.accept(root, origin));
        const uint32_t slot = state.m_first;
        const uint64_t deadline = state.call(slot).m_deadline;
        if (phase != 0)
        {
            PROXY_CHECK(state.prepare(slot, deadline - 5000, output) && !state.finish(slot, NI_OK));
        }
        const bool prepared = state.prepare(slot, deadline + (phase == 2 ? 5000 : 0), output);
        PROXY_CHECK(prepared == (phase == 1));
        if (phase == 1) { PROXY_CHECK(output.m_kind == NI_CANCEL && !state.finish(slot, NI_OK)); }
        PROXY_CHECK(state.m_count == 0 && state.m_bytes == 0);
    }
    // 实际大正文池只有8槽；小正文使用独立分级池，合计超过8MiB仍可接纳。
    proxy.reset();
    std::vector<uint8_t> large(1048576, 0x79);
    PROXY_CHECK(NetInterfaceRpc::encode(NI_REQUEST, 5000, 0, nullptr, 0, large.data(), 16, required) == NI_OK);
    NetInterfaceFrame big = root; big.m_payload = large.data(); big.m_payloadSize = 1048576;
    for (uint32_t index = 0; index < 8; ++index) { big.m_requestId = 700 + index; PROXY_CHECK(proxy.accept(big, origin)); }
    big.m_requestId = 708;
    PROXY_CHECK(!proxy.accept(big, origin) && state.m_count == 8);
    big.m_payloadSize = 144;
    PROXY_CHECK(proxy.accept(big, origin) && state.m_bytes == 8388608);
    big.m_requestId = 709; big.m_payloadSize = 17;
    PROXY_CHECK(proxy.accept(big, origin) && state.m_bytes == 8388609 && state.m_count == 10);
    state.erase(state.m_first);
    big.m_requestId = 710; big.m_payloadSize = 1048576;
    PROXY_CHECK(proxy.accept(big, origin) && state.m_bytes == 8388609 && state.m_count == 10);
    proxy.reset();
    // 输入正文/关联非法必须可观察；零正文不需动态缓冲。
    root.m_requestId = 800;
    PROXY_CHECK(proxy.accept(root, origin) && proxy.accept(root, origin) && !proxy.valid());
    proxy.reset();
    NetInterfaceFrame empty = root; empty.m_payloadSize = 16;
    PROXY_CHECK(proxy.accept(empty, origin) && state.m_bytes == 0 && state.call(state.m_first).m_wire.data() == nullptr);
    PROXY_CHECK(state.prepare(state.m_first, state.call(state.m_first).m_deadline - 5000, output) && output.m_payloadSize == 16);
    PROXY_CHECK(!state.finish(state.m_first, NI_NOT_FOUND) && proxy.valid() && state.m_count == 0);
    PROXY_CHECK(proxy.accept(empty, origin));
    ((service).*netServiceTestMember(NetServiceTestServiceState()))->m_lastAllocated = UINT64_MAX;
    PROXY_CHECK(!state.prepare(state.m_first, state.call(state.m_first).m_deadline - 5000, output) && !proxy.valid() && state.m_count == 0);
    ((service).*netServiceTestMember(NetServiceTestServiceState()))->m_lastAllocated = 0;
#if defined(_WIN32) && defined(_DEBUG)
    for (uint32_t responseMode = 0; memoryAudit && responseMode < 2; ++responseMode)
    {
        uint32_t failures = 0;
        bool reachedSuccess = false;
        for (int step = 0; step < 64 && !reachedSuccess; ++step)
        {
            NetSenderServiceProxy fresh(service);
            NetSenderServiceProxyState& freshState = *((fresh).*netServiceTestMember(NetServiceTestProxyState()));
            root.m_requestId = 900 + step;
            NetInterfaceFrame incoming = root;
            std::vector<uint8_t> responseBytes(1024, 0x59);
            uint32_t expectedBytes = 4;
            if (responseMode)
            {
                PROXY_CHECK(fresh.accept(root, origin));
                PROXY_CHECK(freshState.prepare(freshState.m_first, freshState.call(freshState.m_first).m_deadline - 5000, output));
                incoming = reply; incoming.m_requestId = output.m_requestId;
                PROXY_CHECK(!freshState.finish(freshState.m_first, NI_OK));
                PROXY_CHECK(NetInterfaceRpc::encode(NI_RESPONSE, NI_RPC_SUCCESS, 0, nullptr, 0,
                    responseBytes.data(), 16, required) == NI_OK);
                incoming.m_payload = responseBytes.data(); incoming.m_payloadSize = 1024;
                expectedBytes = 1008;
            }
            s_proxyRejectAfter = step;
            oldHook = _CrtSetAllocHook(ProxyMemoryHook);
            bool caught = false, accepted = false;
            try { accepted = fresh.accept(incoming, origin); } catch (...) { caught = true; }
            _CrtSetAllocHook(oldHook); s_proxyRejectAfter = -1;
            PROXY_CHECK(!caught && fresh.valid() && freshState.m_bytes == (accepted ? expectedBytes : 0U) &&
                freshState.m_count == (accepted || responseMode ? 1U : 0U));
            if (!accepted)
            {
                ++failures;
                PROXY_CHECK(fresh.accept(incoming, origin) && freshState.m_bytes == expectedBytes);
            }
            else { reachedSuccess = true; }
        }
        PROXY_CHECK(failures != 0 && reachedSuccess);
        std::printf("PROXY_ALLOCATION mode=%u failures=%u PASS\n", responseMode, failures);
    }
#endif
    proxy.reset(); root.m_requestId = 1000;
    PROXY_CHECK(proxy.accept(root, origin));
    ((service).*netServiceTestMember(NetServiceTestServiceState()))->m_endpoint = 0;
    proxy.poll();
    PROXY_CHECK(!proxy.valid() && state.m_count == 0 && state.m_bytes == 0 && !state.m_polling);
    proxy.poll(); proxy.reset(); proxy.reset();
    PROXY_CHECK(proxy.valid() && state.m_count == 0);
    // 默认池跨过旧32条和8MiB门槛；实际对象在目录扩容及复用中保持稳定。
    ((service).*netServiceTestMember(NetServiceTestServiceState()))->m_endpoint = 1;
    NetSenderServiceProxy growing(service);
    NetSenderServiceProxyState& grown = *((growing).*netServiceTestMember(NetServiceTestProxyState()));
    for (uint32_t round = 0; round < 2; ++round)
    {
        uint64_t total = 0;
        NetSenderProxyCall* firstAddress = nullptr;
        for (uint32_t index = 0; index < 2048; ++index)
        {
            const uint32_t length = index < 400 ? 16384 : index < 432 ? 131072 : 22;
            std::vector<uint8_t> wire(length, static_cast<uint8_t>(index % 251));
            PROXY_CHECK(NetInterfaceRpc::encode(NI_REQUEST, 60000, 0, nullptr, 0, wire.data(), 16, required) == NI_OK);
            NetInterfaceFrame item = root;
            item.m_requestId = index + 1; item.m_payload = wire.data(); item.m_payloadSize = length;
            PROXY_CHECK(growing.accept(item, origin));
            total += length - 16;
            if (index == 0) { firstAddress = &grown.call(grown.m_first); }
        }
        PROXY_CHECK(grown.m_count == 2048 && grown.m_bytes == total && total > 8388608 &&
            &grown.call(grown.m_first) == firstAddress);
        PROXY_CHECK(growing.configurePools(bodies, records) == NI_INVALID_STATE);
        for (uint32_t index = 0; index < 2048; ++index)
        {
            NetSenderProxyCall& item = grown.call(grown.m_first);
            const uint32_t length = index < 400 ? 16384 : index < 432 ? 131072 : 22;
            PROXY_CHECK(item.m_root == index + 1 && item.m_size == length - 16 &&
                item.m_wire.data()[16] == index % 251 && item.m_wire.data()[length - 1] == index % 251);
            grown.erase(grown.m_first);
        }
        growing.reset();
        PROXY_CHECK(grown.m_count == 0 && grown.m_bytes == 0 && grown.m_first == UINT32_MAX && grown.m_last == UINT32_MAX);
    }
    std::printf("PROXY_POOL rounds=2 frames_per_round=2048 over_8MiB=1 PASS\n");
    std::printf("Service proxy checks=%u\n", checks);
#undef PROXY_CHECK
    return true;
}

int NetSenderServiceProxyTest::audit()
{
    try
    {
#if defined(_WIN32) && defined(_DEBUG)
        _CrtMemState before = {}, after = {};
        _CrtMemCheckpoint(&before);
#endif
        if (!run(true)) { return 1; }
#if defined(_WIN32) && defined(_DEBUG)
        _CrtMemCheckpoint(&after);
        if (after.lCounts[_NORMAL_BLOCK] != before.lCounts[_NORMAL_BLOCK] ||
            after.lSizes[_NORMAL_BLOCK] != before.lSizes[_NORMAL_BLOCK])
        {
            std::puts("Proxy heap mismatch FAIL");
            return 2;
        }
#endif
        std::puts("Proxy storage, allocation rollback and heap PASS");
        return 0;
    }
    catch (...) { return 3; }
}