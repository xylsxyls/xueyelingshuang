#include "NetLocalTestCases.h"
#include "NetWorkTestCases.h"
#include "NetInterfaceTlsTestCases.h"
#include "CSystem/CSystemAPI.h"
#include <chrono>
#include <stdexcept>
#include <sstream>
#ifdef _WIN32
#include <windows.h>
#endif

/** 单调测试时钟
@return 毫秒
*/
static uint64_t LocalTestNow()
{
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

/** 独立断言，不依赖子进程未启动的runner时限
@param [in] condition 条件
@param [in] message 失败说明
*/
static void CheckLocal(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

/** 与实现无关的负载生成规则
@param [in] index 从1开始的消息号
@return 长度
*/
static uint32_t LocalPayloadSize(uint32_t index)
{
    return index % 3 == 0 ? 0 : (index % 3 == 1 ? 1048576 : 8193);
}

NetLocalTestProbe::NetLocalTestProbe(bool server) : m_local(nullptr), m_peer(0), m_holdUntil(0),
    m_received(0), m_closed(0), m_reason(0), m_server(server), m_ready(false), m_bad(false), m_done(false)
{

}

void NetLocalTestProbe::onLocalReady(uint64_t peer, int32_t pid)
{
    m_bad = m_bad || m_ready || pid <= 0;
    m_ready = true;
    m_peer = peer;
}

bool NetLocalTestProbe::onLocalFrame(uint64_t peer, const NetInterfaceFrame& frame)
{
    if (m_server && m_received == 0 && frame.m_kind == NI_REQUEST)
    {
        if (m_holdUntil == 0)
        {
            m_holdUntil = LocalTestNow() + 250;
        }
        if (LocalTestNow() < m_holdUntil)
        {
            return false;
        }
    }
    if (frame.m_kind == NI_EVENT && frame.m_messageType == 999)
    {
        m_bad = m_bad || frame.m_payloadSize != 0 || m_received != 8;
        if (m_server && m_local->send(peer, frame) != NI_OK)
        {
            return false;
        }
        m_done = true;
        return true;
    }
    const uint32_t index = m_received + 1;
    m_bad = m_bad || frame.m_kind != (m_server ? NI_REQUEST : NI_RESPONSE) || frame.m_productId != 4 ||
        frame.m_messageType != 701 || frame.m_requestId != UINT64_C(0x100000000) + index ||
        frame.m_payloadSize != LocalPayloadSize(index);
    for (uint32_t offset = 0; !m_bad && offset < frame.m_payloadSize; ++offset)
    {
        m_bad = frame.m_payload[offset] != static_cast<uint8_t>(index * 17 + offset * 13);
    }
    if (m_server && !m_bad)
    {
        NetInterfaceFrame reply = frame;
        reply.m_kind = NI_RESPONSE;
        const int32_t sent = m_local->send(peer, reply);
        if (sent == NI_LIMIT_EXCEEDED)
        {
            return false;
        }
        m_bad = sent != NI_OK;
    }
    ++m_received;
    return true;
}

void NetLocalTestProbe::onLocalClosed(uint64_t peer, int32_t reason)
{
    m_bad = m_bad || (m_peer != 0 && peer != m_peer);
    ++m_closed;
    m_reason = reason;
}

NetLocalTestHost::NetLocalTestHost(bool server) : m_probe(server), m_local(m_inbox)
{
    m_probe.m_local = &m_local;
    ProcessWork& work = ProcessWork::instance();
    work.addProcessReceiveCallback(&m_inbox);
    if (!work.initReceive(16452, 16))
    {
        work.uninitReceive();
        throw std::runtime_error("local test ProcessWork init");
    }
}

NetLocalTestHost::~NetLocalTestHost()
{
    m_local.shutdown();
    ProcessWork::instance().uninitReceive();
}

#ifdef _WIN32
int NetLocalTestCases::childMain(int32_t parent, int32_t mode)
{
    NetLocalTestHost host(false);
    NetInterfaceTlsOptions options = NetInterfaceTlsTestPair::options(false, "localhost", mode == 2);
    uint64_t peer = 0;
    CheckLocal(host.m_local.connect(parent, options, &host.m_probe, peer) == NI_OK, "local client connect");
    const uint64_t deadline = LocalTestNow() + 9000;
    uint32_t sent = 0;
    bool bye = false;
    bool sawBusy = false;
    while (LocalTestNow() < deadline)
    {
        host.m_local.poll();
        CheckLocal(!host.m_probe.m_bad, "local client invalid response");
        if (mode == 2 && host.m_probe.m_closed != 0)
        {
            return !host.m_probe.m_ready && host.m_probe.m_received == 0 && host.m_probe.m_reason < 0 ? 0 : 21;
        }
        if (mode == 1 && host.m_probe.m_ready && host.m_probe.m_closed == 0)
        {
            while (sent < 8)
            {
                const uint32_t index = sent + 1;
                std::vector<uint8_t> payload(LocalPayloadSize(index));
                for (uint32_t offset = 0; offset < payload.size(); ++offset)
                {
                    payload[offset] = static_cast<uint8_t>(index * 17 + offset * 13);
                }
                NetInterfaceFrame frame = {};
                frame.m_kind = NI_REQUEST;
                frame.m_productId = 4;
                frame.m_requestId = UINT64_C(0x100000000) + index;
                frame.m_messageType = 701;
                frame.m_payload = payload.empty() ? nullptr : payload.data();
                frame.m_payloadSize = static_cast<uint32_t>(payload.size());
                const int32_t queued = host.m_local.send(peer, frame);
                if (queued == NI_LIMIT_EXCEEDED)
                {
                    sawBusy = true;
                    break;
                }
                CheckLocal(queued == NI_OK, "local client queue");
                ++sent;
            }
            if (host.m_probe.m_received == 8 && !bye)
            {
                NetInterfaceFrame frame = {};
                frame.m_kind = NI_EVENT;
                frame.m_messageType = 999;
                CheckLocal(host.m_local.send(peer, frame) == NI_OK, "local client bye");
                bye = true;
            }
            if (host.m_probe.m_done)
            {
                return sawBusy ? 0 : 22;
            }
        }
        if (host.m_probe.m_closed != 0)
        {
            return 23;
        }
        Sleep(1);
    }
    return 24;
}

/** 启动本测试拥有的隐藏子进程，并驱动真实IPC
@param [in] mode 1传输，2错误证书，3对端退出
*/
static void RunLocalChild(int32_t mode)
{
    NetLocalTestHost host(true);
    CheckLocal(host.m_local.listen(NetInterfaceTlsTestPair::options(true, "localhost"), &host.m_probe) == NI_OK,
        "local server listen");
    wchar_t path[32768] = {};
    const DWORD length = GetModuleFileNameW(nullptr, path, 32768);
    CheckLocal(length != 0 && length < 32768, "local child executable path");
    std::wostringstream args;
    args << L'"' << path << L"\" --local-child=" << CSystem::currentProcessPid() << L" --local-mode=" << mode;
    std::wstring command = args.str();
    STARTUPINFOW startup = {};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION child = {};
    CheckLocal(CreateProcessW(path, &command[0], nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
        nullptr, nullptr, &startup, &child) != FALSE, "local child create");
    CloseHandle(child.hThread);
    bool terminated = false;
    try
    {
        while (WaitForSingleObject(child.hProcess, 0) == WAIT_TIMEOUT ||
            (mode == 3 && host.m_probe.m_closed == 0))
        {
            netTestCheckpoint();
            host.m_local.poll();
            CheckLocal(!host.m_probe.m_bad, "local server invalid message");
            if (mode == 3 && host.m_probe.m_ready && !terminated)
            {
                // 只结束本用例创建的进程，然后产生待确认密文，验证固定5秒期限。
                CheckLocal(TerminateProcess(child.hProcess, 31) != FALSE, "terminate owned local child");
                WaitForSingleObject(child.hProcess, 2000);
                terminated = true;
                std::vector<uint8_t> payload(1048576, 0x72);
                NetInterfaceFrame frame = {};
                frame.m_kind = NI_EVENT;
                frame.m_payload = payload.data();
                frame.m_payloadSize = static_cast<uint32_t>(payload.size());
                CheckLocal(host.m_local.send(host.m_probe.m_peer, frame) == NI_OK, "local dead peer queue");
            }
            Sleep(1);
        }
        DWORD exitCode = 0;
        CheckLocal(GetExitCodeProcess(child.hProcess, &exitCode) != FALSE, "local child exit code");
        if (mode == 3)
        {
            CheckLocal(terminated && host.m_probe.m_closed == 1 && host.m_probe.m_reason < 0 &&
                host.m_local.peerCount() == 0, "local dead peer not reclaimed");
        }
        else
        {
            CheckLocal(exitCode == 0, "local child rejected TLS transfer");
            CheckLocal(mode == 2 ? host.m_probe.m_received == 0 : host.m_probe.m_done && host.m_probe.m_received == 8,
                "local transfer incomplete");
        }
    }
    catch (...)
    {
        if (WaitForSingleObject(child.hProcess, 0) == WAIT_TIMEOUT)
        {
            TerminateProcess(child.hProcess, 32);
            WaitForSingleObject(child.hProcess, 2000);
        }
        CloseHandle(child.hProcess);
        throw;
    }
    CloseHandle(child.hProcess);
}

void NetLocalTestCases::transfer()
{
    RunLocalChild(1);
}

void NetLocalTestCases::certificateRejection()
{
    RunLocalChild(2);
}

void NetLocalTestCases::peerExit()
{
    RunLocalChild(3);
}

#endif