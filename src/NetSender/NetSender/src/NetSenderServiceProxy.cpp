#include "NetSenderServiceProxy.h"
#include "NetSenderMacro.h"
#include "NetSenderServiceProxyState.h"
#include "NetInterface/NetInterfaceAPI.h"
#include <chrono>
#include <cstring>

/** 取得本执行者的单调毫秒；期限不使用业务上报UTC
@return 单调毫秒
*/
static uint64_t ProxyNow()
{
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

NetSenderServiceProxy::NetSenderServiceProxy(NetSenderService& service) :
m_state(new NetSenderServiceProxyState(service))
{

}

NetSenderServiceProxy::~NetSenderServiceProxy()
{
    reset();
    delete m_state;
}

bool NetSenderServiceProxy::valid() const
{
    return m_state->m_valid;
}

void NetSenderServiceProxy::reset()
{
    m_state->reset();
}

bool NetSenderServiceProxy::accept(const NetInterfaceFrame& frame, const NetInterfaceOrigin& origin)
{
    if (!m_state->m_valid)
    {
        return true;
    }
    if (origin.m_endpoint == 0)
    {
        m_state->fail();
        return true;
    }
    if (frame.m_kind == NI_CANCEL)
    {
        NetInterfaceFrame unused = {};
        const uint32_t index = m_state->find(frame.m_requestId, true);
        if (NetInterfaceRpc::control(frame, unused) != NI_OK ||
            (index != 32 && !m_state->m_calls[index].matches(frame, origin)))
        {
            m_state->fail();
        }
        else if (index != 32)
        {
            NetSenderProxyCall& call = m_state->m_calls[index];
            if (call.m_phase == NET_SERVICE_PROXY_WAITING)
            {
                call.m_phase = NET_SERVICE_PROXY_CANCEL;
                call.m_cancelUntil = ProxyNow() + 5000;
            }
            else if (call.m_phase != NET_SERVICE_PROXY_CANCEL)
            {
                m_state->erase(index);
            }
        }
        return true;
    }
    NetInterfaceRpcView rpc = {};
    if (NetInterfaceRpc::parse(frame, rpc) != NI_OK)
    {
        m_state->fail();
        return true;
    }
    if (frame.m_kind == NI_RESPONSE)
    {
        const uint32_t index = m_state->find(frame.m_requestId, false);
        if (index == 32)
        {
            return true;
        }
        NetSenderProxyCall& call = m_state->m_calls[index];
        if (!call.matches(frame, origin) ||
            (call.m_phase != NET_SERVICE_PROXY_WAITING && call.m_phase != NET_SERVICE_PROXY_CANCEL))
        {
            m_state->fail();
            return true;
        }
        if (call.m_phase == NET_SERVICE_PROXY_CANCEL)
        {
            m_state->erase(index);
            return true;
        }
        if (rpc.m_size > 8388608 - m_state->m_bytes || !call.assign(rpc))
        {
            return false;
        }
        m_state->m_bytes += rpc.m_size;
        call.m_phase = NET_SERVICE_PROXY_REPLY;
        call.m_status = rpc.m_value;
        return true;
    }
    if (rpc.m_parent != 0 || m_state->find(frame.m_requestId, true) != 32)
    {
        m_state->fail();
        return true;
    }
    if (m_state->m_count == 32 || rpc.m_size > 8388608 - m_state->m_bytes)
    {
        return false;
    }
    uint32_t index = 0;
    while (m_state->m_calls[index].m_phase != NET_SERVICE_PROXY_FREE)
    {
        ++index;
    }
    NetSenderProxyCall& call = m_state->m_calls[index];
    if (!call.assign(rpc))
    {
        return false;
    }
    call.m_origin = origin;
    call.m_root = frame.m_requestId;
    call.m_product = frame.m_productId;
    call.m_type = frame.m_messageType;
    call.m_deadline = ProxyNow() + rpc.m_value;
    call.m_phase = NET_SERVICE_PROXY_QUEUED;
    m_state->append(index);
    return true;
}

void NetSenderServiceProxy::poll()
{
    if (!m_state->m_valid || m_state->m_polling)
    {
        return;
    }
    m_state->m_polling = true;
    bool blocked = false;
    for (uint32_t index = m_state->m_first; m_state->m_valid && index != 32;)
    {
        const uint32_t next = m_state->m_calls[index].m_next;
        NetInterfaceFrame frame = {};
        const uint64_t now = ProxyNow();
        const bool queued = m_state->m_calls[index].m_phase == NET_SERVICE_PROXY_QUEUED;
        if (!(blocked && queued && now < m_state->m_calls[index].m_deadline) && m_state->prepare(index, now, frame))
        {
            int32_t result = NI_INVALID_STATE;
            try
            {
                result = m_state->m_service->send(frame);
            }
            catch (...)
            {
                // 下层未声明异常接纳结果，终止本代；不能盲目重发可能已经接纳的请求。
                m_state->fail();
                break;
            }
            blocked = m_state->finish(index, result) || blocked;
        }
        index = next;
    }
    m_state->m_polling = false;
}

// NET_SERVICE_PROXY_UNIT_BEGIN
// #include "NetSender.h"
// #include "NetSender/NetSenderServiceProxyTest.h"
// #include "NetSender/NetSenderServiceProxyState.h"
// #include "NetSender/NetSenderServiceState.h"
// #include <cstdio>
// #include <cstring>
// #include <vector>
// #if defined(_WIN32) && defined(_DEBUG) && !defined(NET_SERVICE_PROXY_NO_HEAP)
// #include <crtdbg.h>
// static int32_t s_proxyRejectAfter = -1;
// static uint32_t s_proxyAllocations = 0;
// /** 隔离main对普通堆逐位置拒绝分配，或统计热路径分配
// @param [in] operation CRT分配操作；其他参数沿用CRT原型
// @return 是否允许本次分配
// */
// static int __cdecl ProxyMemoryHook(int operation, void*, size_t, int block, long, const unsigned char*, int)
// {
//     if (block == _NORMAL_BLOCK && (operation == _HOOK_ALLOC || operation == _HOOK_REALLOC))
//     {
//         ++s_proxyAllocations;
//         if (s_proxyRejectAfter == 0) { return 0; }
//         if (s_proxyRejectAfter > 0) { --s_proxyRejectAfter; }
//     }
//     return 1;
// }
// #endif
//
// bool NetSenderServiceProxyTest::run()
// {
//     uint32_t checks = 0;
// #define PROXY_CHECK(value) do { ++checks; if (!(value)) { std::printf("Proxy check line=%d FAIL\n", __LINE__); return false; } } while (0)
//     NetSenderLocalInbox inbox;
//     NetSenderService service(inbox);
//     // 白盒只使编号分配器就绪；实际TLS、IPC与转发另由真实进程用例覆盖。
//     service.m_state->m_endpoint = 1;
//     NetSenderServiceProxy proxy(service);
//     NetSenderServiceProxyState& state = *proxy.m_state;
//     NetInterfaceOrigin origin = {};
//     origin.m_endpoint = 91;
//     origin.m_platform = 1;
//     origin.m_device[0] = 37;
//     uint8_t body[4] = {0, 255, 13, 7};
//     uint8_t input[20] = {};
//     uint32_t required = 0;
//     PROXY_CHECK(NetInterfaceRpc::encode(NI_REQUEST, 5000, 0, body, 4, input, sizeof(input), required) == NI_OK);
//     NetInterfaceFrame root = {};
//     root.m_kind = NI_REQUEST; root.m_productId = 9; root.m_messageType = 700;
//     root.m_payload = input; root.m_payloadSize = sizeof(input);
//     NetInterfaceFrame output = {};
//     for (uint32_t index = 0; index < 32; ++index)
//     {
//         root.m_requestId = 100 + index;
//         PROXY_CHECK(proxy.accept(root, origin) && state.m_count == index + 1 && state.m_bytes == (index + 1) * 4);
//     }
//     root.m_requestId = 200;
//     PROXY_CHECK(!proxy.accept(root, origin) && proxy.valid() && state.m_count == 32 && state.m_bytes == 128);
//     PROXY_CHECK(service.m_state->m_lastAllocated == 0);
//     // 中间槽位释放后必须追加至尾，不能按槽位下标打乱根请求顺序。
//     state.erase(3);
//     PROXY_CHECK(proxy.accept(root, origin) && state.m_last == 3 && state.m_first == 0 && state.m_count == 32);
//     uint64_t previous = 0;
//     uint32_t seen = 0;
//     for (uint32_t index = state.m_first; index != 32; index = state.m_calls[index].m_next)
//     {
//         NetSenderProxyCall& call = state.m_calls[index];
//         PROXY_CHECK(state.prepare(index, call.m_deadline - 5000, output));
//         NetInterfaceRpcView rpc = {};
//         PROXY_CHECK(NetInterfaceRpc::parse(output, rpc) == NI_OK && rpc.m_value == 5000 && rpc.m_parent == call.m_root &&
//             rpc.m_size == 4 && std::memcmp(rpc.m_data, body, 4) == 0 && output.m_requestId > previous);
//         previous = output.m_requestId;
//         PROXY_CHECK(!state.finish(index, NI_OK) && call.m_phase == NET_SERVICE_PROXY_WAITING && call.m_size == 0 && !call.m_wire);
//         ++seen;
//     }
//     PROXY_CHECK(seen == 32 && state.m_bytes == 0);
//     proxy.reset();
//     PROXY_CHECK(proxy.valid() && state.m_count == 0 && state.m_first == 32 && state.m_last == 32);
//     root.m_requestId = 300;
//     PROXY_CHECK(proxy.accept(root, origin));
//     NetSenderProxyCall& first = state.m_calls[state.m_first];
//     const uint64_t now = first.m_deadline - 5000;
//     uint64_t lastAttempt = 0;
//     bool valid = true;
// #if defined(_WIN32) && defined(_DEBUG) && !defined(NET_SERVICE_PROXY_NO_HEAP)
//     s_proxyAllocations = 0;
//     _CRT_ALLOC_HOOK oldHook = _CrtSetAllocHook(ProxyMemoryHook);
// #endif
//     for (uint32_t index = 0; index < 10000; ++index)
//     {
//         valid = valid && state.prepare(state.m_first, now, output) && output.m_requestId > lastAttempt;
//         lastAttempt = output.m_requestId;
//         valid = valid && state.finish(state.m_first, NI_LIMIT_EXCEEDED) && first.m_child == 0 && first.m_size == 4;
//     }
// #if defined(_WIN32) && defined(_DEBUG) && !defined(NET_SERVICE_PROXY_NO_HEAP)
//     _CrtSetAllocHook(oldHook);
//     PROXY_CHECK(s_proxyAllocations == 0);
// #endif
//     PROXY_CHECK(valid && state.m_bytes == 4 && state.m_count == 1 && proxy.valid());
//     PROXY_CHECK(state.prepare(state.m_first, now, output));
//     const uint64_t child = output.m_requestId;
//     PROXY_CHECK(!state.finish(state.m_first, NI_OK));
//     NetInterfaceFrame reply = root;
//     reply.m_kind = NI_RESPONSE;
//     reply.m_requestId = child;
//     uint8_t response[20] = {};
//     PROXY_CHECK(NetInterfaceRpc::encode(NI_RESPONSE, NI_RPC_SUCCESS, 0, body, 4, response, 20, required) == NI_OK);
//     reply.m_payload = response;
//     PROXY_CHECK(proxy.accept(reply, origin) && first.m_phase == NET_SERVICE_PROXY_REPLY && state.m_bytes == 4);
//     PROXY_CHECK(state.prepare(state.m_first, now + 100, output) && output.m_kind == NI_RESPONSE && output.m_requestId == root.m_requestId);
//     NetInterfaceRpcView decoded = {};
//     PROXY_CHECK(NetInterfaceRpc::parse(output, decoded) == NI_OK && decoded.m_parent == 0 && decoded.m_size == 4 &&
//         std::memcmp(decoded.m_data, body, 4) == 0 && !state.finish(state.m_first, NI_LIMIT_EXCEEDED));
//     PROXY_CHECK(state.prepare(state.m_first, now + 200, output) && !state.finish(state.m_first, NI_OK) && state.m_count == 0 && state.m_bytes == 0);
//     PROXY_CHECK(proxy.accept(reply, origin) && state.m_count == 0 && proxy.valid());
//     // 请求或响应/取消的来源、产品、命令不匹配，一次性清空本代，不能留下私密正文。
//     for (uint32_t mode = 0; mode < 12; ++mode)
//     {
//         proxy.reset(); root.m_requestId = 400 + mode;
//         PROXY_CHECK(proxy.accept(root, origin));
//         NetInterfaceFrame changed = root;
//         NetInterfaceOrigin changedOrigin = origin;
//         if (mode < 6)
//         {
//             changed.m_kind = NI_CANCEL; changed.m_payload = nullptr; changed.m_payloadSize = 0;
//         }
//         else
//         {
//             PROXY_CHECK(state.prepare(state.m_first, state.m_calls[state.m_first].m_deadline - 5000, output));
//             changed = reply; changed.m_requestId = output.m_requestId;
//             PROXY_CHECK(!state.finish(state.m_first, NI_OK));
//         }
//         switch (mode % 6)
//         {
//         case 0: ++changed.m_productId; break;
//         case 1: ++changed.m_messageType; break;
//         case 2: ++changedOrigin.m_endpoint; break;
//         case 3: ++changedOrigin.m_platform; break;
//         case 4: ++changedOrigin.m_device[3]; break;
//         default: changed.m_payload = nullptr; changed.m_payloadSize = 1; break;
//         }
//         PROXY_CHECK(proxy.accept(changed, changedOrigin) && !proxy.valid() && state.m_count == 0 && state.m_bytes == 0);
//     }
//     // 取消未交付请求不发子取消；已经交付则有限等待，重复取消不延长期限。
//     for (uint32_t mode = 0; mode < 5; ++mode)
//     {
//         proxy.reset(); root.m_requestId = 500 + mode;
//         PROXY_CHECK(proxy.accept(root, origin));
//         const uint32_t slot = state.m_first;
//         NetSenderProxyCall& call = state.m_calls[slot];
//         if (mode != 0)
//         {
//             PROXY_CHECK(state.prepare(slot, call.m_deadline - 5000, output) && !state.finish(slot, NI_OK));
//         }
//         NetInterfaceFrame cancel = root; cancel.m_kind = NI_CANCEL; cancel.m_payload = nullptr; cancel.m_payloadSize = 0;
//         PROXY_CHECK(proxy.accept(cancel, origin));
//         if (mode == 0) { PROXY_CHECK(state.m_count == 0); continue; }
//         const uint64_t until = call.m_cancelUntil;
//         PROXY_CHECK(proxy.accept(cancel, origin) && call.m_cancelUntil == until && call.m_phase == NET_SERVICE_PROXY_CANCEL);
//         if (mode == 4)
//         {
//             reply.m_requestId = call.m_child;
//             PROXY_CHECK(proxy.accept(reply, origin) && state.m_count == 0);
//         }
//         else if (mode == 3)
//         {
//             PROXY_CHECK(!state.prepare(slot, until, output) && state.m_count == 0);
//         }
//         else
//         {
//             PROXY_CHECK(state.prepare(slot, until - 1, output) && output.m_kind == NI_CANCEL && output.m_requestId == call.m_child);
//             PROXY_CHECK(!state.finish(slot, mode == 1 ? NI_OK : NI_NOT_FOUND) && state.m_count == 0);
//         }
//     }
//     for (uint32_t phase = 0; phase < 3; ++phase)
//     {
//         proxy.reset(); root.m_requestId = 600 + phase;
//         PROXY_CHECK(proxy.accept(root, origin));
//         const uint32_t slot = state.m_first;
//         const uint64_t deadline = state.m_calls[slot].m_deadline;
//         if (phase != 0)
//         {
//             PROXY_CHECK(state.prepare(slot, deadline - 5000, output) && !state.finish(slot, NI_OK));
//         }
//         const bool prepared = state.prepare(slot, deadline + (phase == 2 ? 5000 : 0), output);
//         PROXY_CHECK(prepared == (phase == 1));
//         if (phase == 1) { PROXY_CHECK(output.m_kind == NI_CANCEL && !state.finish(slot, NI_OK)); }
//         PROXY_CHECK(state.m_count == 0 && state.m_bytes == 0);
//     }
//     // 最大正文八份再加128字节正好8MiB；超一字节无副作用，移除后可接纳。
//     proxy.reset();
//     std::vector<uint8_t> large(1048576, 0x79);
//     PROXY_CHECK(NetInterfaceRpc::encode(NI_REQUEST, 5000, 0, nullptr, 0, large.data(), 16, required) == NI_OK);
//     NetInterfaceFrame big = root; big.m_payload = large.data(); big.m_payloadSize = 1048576;
//     for (uint32_t index = 0; index < 8; ++index) { big.m_requestId = 700 + index; PROXY_CHECK(proxy.accept(big, origin)); }
//     big.m_requestId = 708; big.m_payloadSize = 144;
//     PROXY_CHECK(proxy.accept(big, origin) && state.m_bytes == 8388608);
//     big.m_requestId = 709; big.m_payloadSize = 17;
//     PROXY_CHECK(!proxy.accept(big, origin) && state.m_bytes == 8388608 && state.m_count == 9);
//     state.erase(state.m_first);
//     PROXY_CHECK(proxy.accept(big, origin) && state.m_bytes == 8388608 - 1048560 + 1);
//     proxy.reset();
//     // 输入正文/关联非法必须可观察；零正文不需动态缓冲。
//     root.m_requestId = 800;
//     PROXY_CHECK(proxy.accept(root, origin) && proxy.accept(root, origin) && !proxy.valid());
//     proxy.reset();
//     NetInterfaceFrame empty = root; empty.m_payloadSize = 16;
//     PROXY_CHECK(proxy.accept(empty, origin) && state.m_bytes == 0 && !state.m_calls[state.m_first].m_wire);
//     PROXY_CHECK(state.prepare(state.m_first, state.m_calls[state.m_first].m_deadline - 5000, output) && output.m_payloadSize == 16);
//     PROXY_CHECK(!state.finish(state.m_first, NI_NOT_FOUND) && proxy.valid() && state.m_count == 0);
//     PROXY_CHECK(proxy.accept(empty, origin));
//     service.m_state->m_lastAllocated = UINT64_MAX;
//     PROXY_CHECK(!state.prepare(state.m_first, state.m_calls[state.m_first].m_deadline - 5000, output) && !proxy.valid() && state.m_count == 0);
//     service.m_state->m_lastAllocated = 0;
// #if defined(_WIN32) && defined(_DEBUG) && !defined(NET_SERVICE_PROXY_NO_HEAP)
//     for (uint32_t responseMode = 0; responseMode < 2; ++responseMode)
//     {
//         for (int step = 0; step < 8; ++step)
//         {
//             proxy.reset(); root.m_requestId = 900 + step;
//             NetInterfaceFrame incoming = root;
//             if (responseMode)
//             {
//                 PROXY_CHECK(proxy.accept(root, origin));
//                 PROXY_CHECK(state.prepare(state.m_first, state.m_calls[state.m_first].m_deadline - 5000, output));
//                 incoming = reply; incoming.m_requestId = output.m_requestId;
//                 PROXY_CHECK(!state.finish(state.m_first, NI_OK));
//             }
//             s_proxyRejectAfter = step;
//             oldHook = _CrtSetAllocHook(ProxyMemoryHook);
//             bool caught = false, accepted = false;
//             try { accepted = proxy.accept(incoming, origin); } catch (...) { caught = true; }
//             _CrtSetAllocHook(oldHook); s_proxyRejectAfter = -1;
//             PROXY_CHECK(!caught && proxy.valid() && state.m_bytes == (accepted ? 4U : 0U) &&
//                 state.m_count == (accepted || responseMode ? 1U : 0U));
//             if (step == 0) { PROXY_CHECK(!accepted && proxy.accept(incoming, origin) && state.m_bytes == 4); }
//         }
//     }
// #endif
//     proxy.reset(); root.m_requestId = 1000;
//     PROXY_CHECK(proxy.accept(root, origin));
//     service.m_state->m_endpoint = 0;
//     proxy.poll();
//     PROXY_CHECK(!proxy.valid() && state.m_count == 0 && state.m_bytes == 0 && !state.m_polling);
//     proxy.poll(); proxy.reset(); proxy.reset();
//     PROXY_CHECK(proxy.valid() && state.m_count == 0);
//     std::printf("Service proxy checks=%u\n", checks);
// #undef PROXY_CHECK
//     return true;
// }
//
// #ifndef NET_SERVICE_PROXY_NO_MAIN
// int main()
// {
//     try
//     {
// #if defined(_WIN32) && defined(_DEBUG) && !defined(NET_SERVICE_PROXY_NO_HEAP)
//         _CrtMemState before = {}, after = {};
//         _CrtMemCheckpoint(&before);
// #endif
//         if (!NetSenderServiceProxyTest::run())
//{
//    return 1;
//}

// #if defined(_WIN32) && defined(_DEBUG) && !defined(NET_SERVICE_PROXY_NO_HEAP)
//         _CrtMemCheckpoint(&after);
//         const long long blocks = static_cast<long long>(after.lCounts[_NORMAL_BLOCK]) - before.lCounts[_NORMAL_BLOCK];
//         const long long bytes = static_cast<long long>(after.lSizes[_NORMAL_BLOCK]) - before.lSizes[_NORMAL_BLOCK];
//         if (blocks != 0 || bytes != 0)
//         {
//             std::printf("Service proxy heap blocks=%lld bytes=%lld FAIL\n", blocks, bytes);
//             return 2;
//         }
// #endif
//         std::puts("Service proxy fixed FIFO, budget and exception recovery PASS");
//         return 0;
//     }
//     catch (...)
//     {
//         return 3;
//     }
// }
// #endif
// NET_SERVICE_PROXY_UNIT_END