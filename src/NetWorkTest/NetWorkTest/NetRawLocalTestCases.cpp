#include "NetRawLocalTestCases.h"
#include "NetInterfaceTlsTestCases.h"
#include "NetWorkTestCases.h"
#include "CSystem/CSystemAPI.h"
#include <chrono>
#include <cstdio>
#include <sstream>
#ifdef _WIN32
#include <windows.h>
#endif

/** 独立测试单调时间
@return 毫秒
*/
static uint64_t RawLocalNow()
{
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

NetRawLocalTestInbox::NetRawLocalTestInbox(bool gateEnd, bool dropEnd) :
m_gateEnd(gateEnd),
m_dropEnd(dropEnd),
m_waiting(false),
m_release(false)
{

}

void NetRawLocalTestInbox::receive(int32_t sendPid, const char* buffer, int32_t length, MessageType type)
{
    if ((m_gateEnd || m_dropEnd) && static_cast<int32_t>(type) == NET_SENDER_LOCAL_MESSAGE && buffer != nullptr && length == 56)
    {
        NetSenderLocalPacketView packet = {};
        if (NetSenderLocalPacket::parse(reinterpret_cast<const uint8_t*>(buffer), 56, packet) == NI_OK &&
            packet.m_kind == NET_LOCAL_END)
        {
            m_waiting.store(true, std::memory_order_release);
            if (m_dropEnd)
            {
                return;
            }
            const uint64_t deadline = RawLocalNow() + 2000;
            while (!m_release.load(std::memory_order_acquire) && RawLocalNow() < deadline)
            {
                CSystem::Sleep(1);
            }
        }
    }
    NetSenderLocalInbox::receive(sendPid, buffer, length, type);
}

NetRawLocalTestProbe::NetRawLocalTestProbe(bool server) : m_local(nullptr), m_peer(0), m_holdUntil(0), m_received(0),
    m_closed(0), m_reason(0), m_expectedReason(NI_OK), m_server(server), m_ready(false), m_hello(false), m_bad(false)
{

}

void NetRawLocalTestProbe::onRawLocalReady(uint64_t peer, int32_t pid)
{
    m_bad = m_bad || peer == 0 || pid <= 0 || m_ready;
    m_peer = peer;
    m_ready = true;
}

bool NetRawLocalTestProbe::onRawLocalRecord(uint64_t peer, const NetInterfaceTunnelRecord& record)
{
    if (m_server && RawLocalNow() < m_holdUntil) { return false; }
    if (peer != m_peer) { m_bad = true; return true; }
    if (record.m_kind == NI_TUNNEL_HELLO)
    {
        if (m_hello) { m_bad = true; return true; }
    }
    else
    {
        const uint32_t index = m_received + 1;
        if (!m_hello || index > 8 || record.m_kind != NI_TUNNEL_BYTES || record.m_product != 4 ||
            record.m_channel != UINT64_C(0x1234567800000000) + index || record.m_size != 16384)
        {
            m_bad = true;
            return true;
        }
        for (uint32_t offset = 0; offset < record.m_size; ++offset)
        {
            if (record.m_data[offset] != static_cast<uint8_t>(index * 17 + offset * 13)) { m_bad = true; return true; }
        }
    }
    if (m_server)
    {
        const int32_t sent = m_local->sendRecord(peer, record);
        if (sent == NI_LIMIT_EXCEEDED) { return false; }
        if (sent != NI_OK) { m_bad = true; return true; }
    }
    if (record.m_kind == NI_TUNNEL_HELLO) { m_hello = true; }
    else { ++m_received; }
    return true;
}

void NetRawLocalTestProbe::onRawLocalClosed(uint64_t peer, int32_t reason)
{
    m_bad = m_bad || peer != m_peer || reason != m_expectedReason;
    m_reason = reason;
    ++m_closed;
}

NetRawLocalTestHost::NetRawLocalTestHost(bool server, bool gateEnd, bool dropEnd) : m_inbox(gateEnd, dropEnd), m_probe(server), m_local(m_inbox)
{
    m_probe.m_local = &m_local;
    ProcessWork::instance().addProcessReceiveCallback(&m_inbox);
    if (!ProcessWork::instance().initReceive(16452, 16))
    {
        ProcessWork::instance().uninitReceive();
        throw std::runtime_error("raw local test ProcessWork init failed");
    }
}

NetRawLocalTestHost::~NetRawLocalTestHost()
{
    m_inbox.m_release.store(true, std::memory_order_release);
    m_local.shutdown();
    ProcessWork::instance().uninitReceive();
}

#ifdef _WIN32
int NetRawLocalTestCases::childMain(int32_t parent)
{
    NetRawLocalTestHost host(false);
    uint64_t peer = 0;
    NetInterfaceTlsTestPair::require(host.m_local.connectRaw(parent, NetInterfaceTlsTestPair::strongRandom,
        nullptr, &host.m_probe, peer) == NI_OK, "raw IPC child connect failed");
    const uint64_t deadline = RawLocalNow() + 9000;
    bool hello = false;
    bool sawBusy = false;
    uint32_t sent = 0;
    while (RawLocalNow() < deadline)
    {
        host.m_local.poll();
        NetInterfaceTlsTestPair::require(!host.m_probe.m_bad, "raw IPC child received invalid data");
        if (host.m_probe.m_ready && !hello)
        {
            NetInterfaceTunnelRecord record = {};
            record.m_kind = NI_TUNNEL_HELLO;
            NetInterfaceTlsTestPair::require(host.m_local.sendRecord(peer, record) == NI_OK, "raw IPC child hello failed");
            hello = true;
        }
        if (host.m_probe.m_hello && host.m_probe.m_closed == 0)
        {
            while (sent < 8)
            {
                const uint32_t index = sent + 1;
                std::vector<uint8_t> payload(16384);
                for (uint32_t offset = 0; offset < payload.size(); ++offset)
                {
                    payload[offset] = static_cast<uint8_t>(index * 17 + offset * 13);
                }
                NetInterfaceTunnelRecord record = {};
                record.m_kind = NI_TUNNEL_BYTES;
                record.m_product = 4;
                record.m_channel = UINT64_C(0x1234567800000000) + index;
                record.m_data = payload.data();
                record.m_size = static_cast<uint32_t>(payload.size());
                const int32_t queued = host.m_local.sendRecord(peer, record);
                if (queued == NI_LIMIT_EXCEEDED) { sawBusy = true; break; }
                NetInterfaceTlsTestPair::require(queued == NI_OK, "raw IPC child send failed");
                ++sent;
            }
            if (host.m_probe.m_received == 8)
            {
                host.m_local.close(peer, 0);
                host.m_local.poll();
                return sawBusy && host.m_probe.m_closed == 1 && !host.m_probe.m_bad ? 0 : 21;
            }
        }
        if (host.m_probe.m_closed != 0) { return 22; }
        CSystem::Sleep(1);
    }
    return 23;
}

void NetRawLocalTestCases::transfer()
{
    runTransfer(0);
}

void NetRawLocalTestCases::delayedEnd()
{
    runTransfer(1);
}

void NetRawLocalTestCases::missingEnd()
{
    runTransfer(2);
}

void NetRawLocalTestCases::runTransfer(uint32_t mode)
{
    const bool gateEnd = mode == 1;
    const bool dropEnd = mode == 2;
    NetRawLocalTestHost host(true, gateEnd, dropEnd);
    host.m_probe.m_expectedReason = dropEnd ? NI_TRUNCATED : NI_OK;
    NetInterfaceTlsTestPair::require(host.m_local.listenRaw(NetInterfaceTlsTestPair::strongRandom, nullptr,
        &host.m_probe) == NI_OK, "raw IPC server listen failed");
    host.m_probe.m_holdUntil = RawLocalNow() + 150;
    wchar_t path[32768] = {};
    const DWORD length = GetModuleFileNameW(nullptr, path, 32768);
    NetInterfaceTlsTestPair::require(length != 0 && length < 32768, "raw IPC test executable path");
    std::wostringstream argument;
    argument << L'"' << path << L"\" --raw-local-child=" << CSystem::currentProcessPid();
    std::wstring command = argument.str();
    STARTUPINFOW startup = {};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    startup.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    startup.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    PROCESS_INFORMATION child = {};
    NetInterfaceTlsTestPair::require(CreateProcessW(path, &command[0], nullptr, nullptr, TRUE, CREATE_NO_WINDOW,
        nullptr, nullptr, &startup, &child) != FALSE, "create owned raw IPC child failed");
    CloseHandle(child.hThread);
    bool success = false;
    DWORD code = 1;
    uint64_t gateSince = 0;
    uint64_t exitedAt = 0;
    bool testedExitBeforeEnd = false;
    try
    {
        const uint64_t deadline = RawLocalNow() + 10000;
        while (RawLocalNow() < deadline)
        {
            netTestCheckpoint();
            host.m_local.poll();
            if (host.m_probe.m_bad) { break; }
            if (WaitForSingleObject(child.hProcess, 0) == WAIT_OBJECT_0)
            {
                GetExitCodeProcess(child.hProcess, &code);
                if (exitedAt == 0)
                {
                    exitedAt = RawLocalNow();
                }
                if (gateEnd && !host.m_inbox.m_release.load(std::memory_order_acquire))
                {
                    if (host.m_inbox.m_waiting.load(std::memory_order_acquire))
                    {
                        if (gateSince == 0)
                        {
                            gateSince = RawLocalNow();
                        }
                        if (RawLocalNow() - gateSince >= 100)
                        {
                            testedExitBeforeEnd = host.m_probe.m_closed == 0 && !host.m_probe.m_bad;
                            host.m_inbox.m_release.store(true, std::memory_order_release);
                        }
                    }
                    CSystem::Sleep(1);
                    continue;
                }
                if (host.m_probe.m_closed == 0)
                {
                    CSystem::Sleep(1);
                    continue;
                }
                success = code == 0 && host.m_probe.m_received == 8 && host.m_probe.m_hello &&
                    host.m_probe.m_closed == 1 && !host.m_probe.m_bad && (!gateEnd || testedExitBeforeEnd) &&
                    (!dropEnd || (host.m_inbox.m_waiting.load(std::memory_order_acquire) && RawLocalNow() - exitedAt >= 4000));
                break;
            }
            CSystem::Sleep(1);
        }
    }
    catch (...)
    {
        TerminateProcess(child.hProcess, 1);
        WaitForSingleObject(child.hProcess, 2000);
        CloseHandle(child.hProcess);
        throw;
    }
    if (!success && WaitForSingleObject(child.hProcess, 0) == WAIT_TIMEOUT)
    {
        TerminateProcess(child.hProcess, 1);
        WaitForSingleObject(child.hProcess, 2000);
    }
    CloseHandle(child.hProcess);
    if (!success)
    {
        std::printf("RAW_LOCAL_FAILURE child=%u received=%u hello=%u closed=%u reason=%d bad=%u\n",
            static_cast<uint32_t>(code), host.m_probe.m_received, host.m_probe.m_hello ? 1U : 0U,
            host.m_probe.m_closed, host.m_probe.m_reason, host.m_probe.m_bad ? 1U : 0U);
    }
    if (gateEnd || dropEnd)
    {
        std::printf("RAW_LOCAL_DRAIN end_observed=%u delayed_exit_verified=%u expectation_met=%u\n",
            host.m_inbox.m_waiting.load(std::memory_order_acquire) ? 1U : 0U,
            testedExitBeforeEnd ? 1U : 0U, success ? 1U : 0U);
    }
    NetInterfaceTlsTestPair::require(success, "raw IPC complete records/backpressure/closure failed");
}

#endif