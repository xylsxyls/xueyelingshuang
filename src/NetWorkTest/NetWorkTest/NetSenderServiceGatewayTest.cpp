#define NET_SERVICE_GATEWAY_NO_HEAP
#include <LibuvTcp/LibuvTcpAPI.h>
#include "NetWorkConcurrencyTestCases.h"
#include <NetWork/NetWorkAPI.h>
#include <NetSender/NetSenderAPI.h>
#include <test/certs.h>
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

/** 建立真实TLS拥有者但不联网，供停止/析构路径测试
@param [in,out] state 实际服务状态
@param [in] count 需要回收的客户端数
@return 是否创建成功
*/
static bool ServiceGatewayCreateClients(NetSenderServiceGatewayState& state, uint32_t count)
{
    for (uint32_t index = 0; index < count; ++index)
    {
        NetInterfaceTlsOptions tls = {};
        tls.entropy = CSystem::secureRandomBytesCallback;
        tls.handshakeTimeoutMs = 5000;
        tls.authority = reinterpret_cast<const uint8_t*>(mbedtls_test_ca_crt_rsa_sha256_pem);
        tls.authoritySize = static_cast<uint32_t>(mbedtls_test_ca_crt_rsa_sha256_pem_len);
        tls.hostname = "localhost";
        std::unique_ptr<NetSenderServiceGatewayClient> client(new NetSenderServiceGatewayClient(tls, NI_LINK_CLIENT));
        if (client->m_connection.status() < 0)
        {
            return false;
        }
        state.m_clients.insert(std::make_pair(index + 1, std::move(client)));
    }
    return true;
}

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif

#if defined(_WIN32) && defined(_DEBUG) && !defined(NET_SERVICE_GATEWAY_NO_HEAP)
#include <crtdbg.h>
static int32_t s_serviceGatewayRejectAfter = -1;
static uint32_t s_serviceGatewayAllocations = 0;

/** 本隔离main内逐次拒绝普通分配，验证注册和关闭代码失败后不会卡在polling
@param [in] operation CRT操作
@param [in] block 堆块类别；其他参数遵从CRT原型
@param [in] pointer CRT传入的堆块，只在钩子返回前借用；本用例未使用时忽略
@param [in] size CRT请求的字节数，是否参与注入判定由本用例决定
@param [in] request CRT分配序号，本用例不使用
@param [in] filename CRT传入的来源文件名，本用例不读取
@param [in] line 来源代码行号，本用例不使用
@return 是否允许本次操作
*/
static int __cdecl ServiceGatewayMemoryHook(int operation, void* pointer, size_t size, int block, long request, const unsigned char* filename, int line)
{
    (void)pointer;
    (void)size;
    (void)request;
    (void)filename;
    (void)line;
    if (block == _NORMAL_BLOCK && (operation == _HOOK_ALLOC || operation == _HOOK_REALLOC))
    {
        ++s_serviceGatewayAllocations;
        if (s_serviceGatewayRejectAfter == 0)
        {
            return 0;
        }
        if (s_serviceGatewayRejectAfter > 0)
        {
            --s_serviceGatewayRejectAfter;
        }
    }
    return 1;
}
#endif

#include "NetSender/NetSenderAPI.h"
#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif

NetSenderServiceGatewayTest::NetSenderServiceGatewayTest(NetSenderServiceGatewayState& state) :
m_state(&state),
m_clients(0),
m_services(0),
m_throw(false),
m_bad(false)
{

}

void NetSenderServiceGatewayTest::onGatewayServiceReady()
{
    m_bad = true;
}

void NetSenderServiceGatewayTest::onGatewayClientReady(uint64_t channel, uint32_t role, const NetInterfaceOrigin& origin)
{
    (void)channel;
    (void)role;
    (void)origin;
    m_bad = true;
}

bool NetSenderServiceGatewayTest::onGatewayClientFrame(uint64_t channel, uint32_t role,
    const NetInterfaceFrame& frame, const NetInterfaceOrigin& origin)
{
    (void)channel;
    (void)role;
    (void)frame;
    (void)origin;
    m_bad = true;
    return true;
}

void NetSenderServiceGatewayTest::onGatewayClientClosed(uint64_t channel, int32_t reason)
{
    (void)reason;
    ++m_clients;
    m_bad = m_bad || m_state->m_clients.count(channel) != 0 || m_services != 0;
    if (m_state->closing())
    {
        m_bad = m_bad || m_state->poll();
    }
    if (m_throw)
    {
        throw 7;
    }
}

void NetSenderServiceGatewayTest::onGatewayServiceClosed(int32_t reason)
{
    ++m_services;
    m_bad = m_bad || !m_state->m_clients.empty() || m_state->m_phase != NET_SERVICE_CLOSED ||
        m_state->m_reason != reason || !m_state->m_key.empty() || m_state->poll();
    m_state->fail(NI_PROTOCOL_ERROR);
    m_bad = m_bad || m_state->m_reason != reason;
    if (m_throw)
    {
        throw 8;
    }
}

bool NetSenderServiceGatewayTest::run()
{
    const uint64_t now = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
    uint32_t cycles = 1;
#if defined(_WIN32) && defined(_DEBUG) && !defined(NET_SERVICE_GATEWAY_NO_HEAP)
    cycles = 32;
#endif
    for (uint32_t mode = 0; mode < 3; ++mode)
    {
        for (uint32_t step = 0; step < cycles; ++step)
        {
            NetSenderLocalInbox inbox;
            NetSenderServiceGatewayState state(inbox);
            NetSenderServiceGatewayTest listener(state);
            state.m_listener = &listener;
            state.m_phase = mode == 0 ? NET_SERVICE_AUTHENTICATED : NET_SERVICE_READY;
            state.m_channels = 8;
            state.m_weight = 1;
            state.m_key.assign(64, 'q');
            if (mode == 1)
            {
                state.m_outgoingClose = 1;
            }
            if (mode == 2)
            {
                if (!ServiceGatewayCreateClients(state, 2))
                {
                    return false;
                }
                state.close(1, NI_CANCELLED);
                state.close(2, NI_CANCELLED, false);
            }
            bool escaped = false;
#if defined(_WIN32) && defined(_DEBUG) && !defined(NET_SERVICE_GATEWAY_NO_HEAP)
            s_serviceGatewayRejectAfter = static_cast<int32_t>(step);
            const _CRT_ALLOC_HOOK previous = _CrtSetAllocHook(ServiceGatewayMemoryHook);
#endif
            try
            {
                state.poll();
            }
            catch (...)
            {
                escaped = true;
            }
#if defined(_WIN32) && defined(_DEBUG) && !defined(NET_SERVICE_GATEWAY_NO_HEAP)
            _CrtSetAllocHook(previous);
            s_serviceGatewayRejectAfter = -1;
            if (step == 0 && state.m_reason != NI_NO_MEMORY)
            {
                return false;
            }
#endif
            if (escaped || state.m_polling || state.m_phase != NET_SERVICE_CLOSED || state.poll() ||
                listener.m_bad || listener.m_services != 1 || listener.m_clients != (mode == 2 ? 2U : 0U))
            {
                std::printf("Service gateway failure mode=%u step=%u escaped=%u phase=%u polling=%u clients=%u services=%u\n",
                    mode, step, escaped ? 1U : 0U, state.m_phase, state.m_polling ? 1U : 0U, listener.m_clients, listener.m_services);
                return false;
            }
        }
    }
    for (uint32_t throws = 0; throws < 2; ++throws)
    {
        NetSenderLocalInbox inbox;
        NetSenderServiceGatewayState state(inbox);
        NetSenderServiceGatewayTest listener(state);
        listener.m_throw = throws != 0;
        state.m_listener = &listener;
        state.m_phase = NET_SERVICE_READY;
        if (!ServiceGatewayCreateClients(state, 8))
        {
            return false;
        }
        state.fail(NI_CANCELLED);
        state.fail(NI_PROTOCOL_ERROR);
#if defined(_WIN32) && defined(_DEBUG) && !defined(NET_SERVICE_GATEWAY_NO_HEAP)
        s_serviceGatewayRejectAfter = 0;
        s_serviceGatewayAllocations = 0;
        const _CRT_ALLOC_HOOK previous = _CrtSetAllocHook(ServiceGatewayMemoryHook);
#endif
        const bool active = state.poll();
#if defined(_WIN32) && defined(_DEBUG) && !defined(NET_SERVICE_GATEWAY_NO_HEAP)
        _CrtSetAllocHook(previous);
        s_serviceGatewayRejectAfter = -1;
        if (s_serviceGatewayAllocations != 0)
        {
            return false;
        }
#endif
        if (active || state.m_polling || state.m_phase != NET_SERVICE_CLOSED || state.m_reason != NI_CANCELLED ||
            listener.m_bad || listener.m_clients != 8 || listener.m_services != 1)
        {
            return false;
        }
    }
    {
        NetSenderLocalInbox inbox;
        NetSenderServiceGatewayState state(inbox);
        NetSenderServiceGatewayTest listener(state);
        state.m_listener = &listener;
        if (state.poll() || state.m_polling || state.closing())
        {
            return false;
        }
        state.m_phase = NET_SERVICE_READY;
        if (!ServiceGatewayCreateClients(state, 2))
        {
            return false;
        }
        state.close(1, NI_CANCELLED, false);
        state.collectClosed(now);
        if (listener.m_clients != 1 || listener.m_services != 0 || state.m_clients.count(2) != 1 ||
            state.m_phase != NET_SERVICE_READY || listener.m_bad)
        {
            return false;
        }
        state.fail(0);
        if (state.poll() || listener.m_clients != 2 || listener.m_services != 1 || listener.m_bad)
        {
            return false;
        }
    }
    {
        NetSenderLocalInbox inbox;
        NetSenderServiceGatewayState state(inbox);
        NetSenderServiceGatewayTest listener(state);
        state.m_listener = &listener;
        state.m_phase = NET_SERVICE_READY;
        listener.m_throw = true;
        if (!ServiceGatewayCreateClients(state, 2))
        {
            return false;
        }
        // 先略过活动的1，再在2的终态回调中失败；必须在本轮回收1，不能遗留到析构。
        state.close(2, NI_CANCELLED, false);
        state.m_polling = true;
        state.collectClosed(now);
        state.m_polling = false;
        if (!state.m_clients.empty() || listener.m_clients != 2 || listener.m_bad ||
            state.m_reason != NI_INVALID_STATE || state.poll() || listener.m_services != 1)
        {
            return false;
        }
    }
    std::printf("Service gateway lifecycle fault_cycles=%u PASS\n", cycles * 3);
    return true;
}

bool NetSenderServiceGatewayTest::clockBoundary()
{
    NetSenderLocalInbox inbox;
    NetSenderServiceGatewayState state(inbox);
    state.m_phase = NET_SERVICE_READY;
    NetInterfaceTlsOptions tls = {};
    tls.server = 1;
    tls.entropy = CSystem::secureRandomBytesCallback;
    tls.handshakeTimeoutMs = 60000;
    tls.certificate = reinterpret_cast<const uint8_t*>(mbedtls_test_srv_crt_rsa_sha256_pem);
    tls.certificateSize = static_cast<uint32_t>(mbedtls_test_srv_crt_rsa_sha256_pem_len);
    tls.privateKey = reinterpret_cast<const uint8_t*>(mbedtls_test_srv_key_rsa_pem);
    tls.privateKeySize = static_cast<uint32_t>(mbedtls_test_srv_key_rsa_pem_len);
    std::unique_ptr<NetSenderServiceGatewayClient> entry(new NetSenderServiceGatewayClient(tls, NI_LINK_CLIENT));
    if (entry->m_connection.status() < 0) { return false; }
    state.m_clients.insert(std::make_pair(UINT64_C(1), std::move(entry)));
    NetSenderServiceGatewayClient& client = *state.m_clients.begin()->second;
    // 模拟link.poll回调在本轮时间之后1ms更新活动，不伪装TLS已握手。
    client.m_lastReceive = 1001;
    const uint64_t times[] = {1000, 1001, 46000, 46001};
    for (uint32_t index = 0; index < 4; ++index)
    {
        state.pump(1, client, times[index]);
        if (client.m_closing != (index == 3) || (index == 3 && client.m_reason != NI_TIMEOUT))
        {
            std::printf("Service clock now=%llu last=%llu closed=%u reason=%d FAIL\n",
                static_cast<unsigned long long>(times[index]), static_cast<unsigned long long>(client.m_lastReceive),
                client.m_closing ? 1U : 0U, client.m_reason);
            return false;
        }
    }
    return true;
}