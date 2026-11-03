#include "NetSenderTestClient.h"
#include "NetServiceTestCases.h"
#ifdef _WIN32
#include "NetGatewayTestCases.h"
#include "NetInterfaceTlsTestCases.h"
#include "NetWorkTestCases.h"
#include "../../NetServerManager/NetServerManager/NetGatewayLocalServer.h"
#include "CSystem/CSystemAPI.h"
#include <chrono>
#include <cstring>
#include <sstream>
#include <stdexcept>

/** 独立断言，可用于未启用主runner的子进程
@param [in] expected 条件
@param [in] message 诊断
*/
static void CheckService(bool expected, const char* message)
{
    if (!expected)
    {
        throw std::runtime_error(message);
    }
}

/** 隔离测试信号名
@param [in] prefix 信号用途
@param [in] pid 父进程
@param [in] product 产品
@param [in] mode 模式
@return 本次独占名称
*/
static std::string ServiceSignalName(const char* prefix, int32_t pid, uint32_t product, uint32_t mode)
{
    std::ostringstream name;
    name << "NetServiceTest_" << prefix << '_' << pid << '_' << product << '_' << mode;
    return name.str();
}

/** 公开测试凭据，不部署或作为生产默认
@param [in] product 0/4
@return 固定测试秘密
*/
static NetGatewayServiceCredential TestCredential(uint32_t product)
{
    NetGatewayServiceCredential credential = {};
    credential.m_product = product;
    for (uint32_t index = 0; index < 32; ++index)
    {
        credential.m_secret[index] = static_cast<uint8_t>(index * 7 + product * 31 + 1);
    }
    return credential;
}

NetServiceTestProbe::NetServiceTestProbe(uint32_t product, Semaphore& ready) : m_service(nullptr), m_readySignal(&ready),
    m_product(product), m_expectedPlatform(3), m_ready(false), m_closed(false), m_bad(false), m_pending(false), m_origin(), m_outer(), m_accountChild(0)
{

}

void NetServiceTestProbe::onServiceReady(uint64_t endpoint)
{
    m_bad = endpoint == 0 || m_ready;
    m_ready = true;
    m_bad = m_bad || m_readySignal->tryProcessSignal() != 0;
}

bool NetServiceTestProbe::onServiceFrame(const NetInterfaceFrame& frame, const NetInterfaceOrigin& origin)
{
    NetInterfaceRpcView rpc = {};
    m_bad = m_bad || NetInterfaceRpc::parse(frame, rpc) != NI_OK || frame.m_productId != 4 ||
        origin.m_endpoint == 0 || origin.m_platform != m_expectedPlatform || origin.m_device[0] != 0x42;
    if (m_bad)
    {
        return true;
    }
    NetInterfaceFrame next = frame;
    uint64_t parent = 0;
    uint32_t status = NI_RPC_SUCCESS;
    if (m_product == 4 && frame.m_kind == NI_REQUEST)
    {
        const bool account = frame.m_messageType >= ACCOUNT_REGISTER_PASSWORD && (frame.m_messageType <= ACCOUNT_RESET_PASSWORD && frame.m_messageType != ACCOUNT_EVENT_SESSION_REPLACED);
        m_bad = m_pending || (!account && frame.m_messageType != 700);
        if (account)
        {
            CheckService(m_service->nextRequestId(next.m_requestId) == NI_OK, "account proxy child request id");
        }
        else
        {
            next.m_requestId = UINT64_C(0x8123456789abcdef);
            next.m_messageType = 900;
        }
        parent = frame.m_requestId;
        status = rpc.m_value;
    }
    else if (m_product == 0 && frame.m_kind == NI_REQUEST)
    {
        m_bad = frame.m_messageType != 900 || rpc.m_parent == 0;
        next.m_kind = NI_RESPONSE;
    }
    else if (m_product == 4 && frame.m_kind == NI_RESPONSE)
    {
        const bool account = frame.m_messageType >= ACCOUNT_REGISTER_PASSWORD && (frame.m_messageType <= ACCOUNT_RESET_PASSWORD && frame.m_messageType != ACCOUNT_EVENT_SESSION_REPLACED);
        m_bad = !m_pending || frame.m_requestId != (account ? m_accountChild : UINT64_C(0x8123456789abcdef)) ||
            (!account && frame.m_messageType != 900) || rpc.m_value != NI_RPC_SUCCESS ||
            origin.m_endpoint != m_origin.m_endpoint || std::memcmp(origin.m_device, m_origin.m_device, 16) != 0;
        next = m_outer;
        next.m_kind = NI_RESPONSE;
    }
    else
    {
        m_bad = true;
    }
    if (m_bad)
    {
        return true;
    }
    std::vector<uint8_t> bytes(rpc.m_size + 16);
    uint32_t required = 0;
    CheckService(NetInterfaceRpc::encode(next.m_kind, status, parent, rpc.m_data, rpc.m_size,
        bytes.data(), static_cast<uint32_t>(bytes.size()), required) == NI_OK, "service test response encode");
    next.m_payload = bytes.data();
    next.m_payloadSize = required;
    const int32_t sent = m_service->send(next);
    if (sent == NI_LIMIT_EXCEEDED)
    {
        return false;
    }
    m_bad = sent != NI_OK;
    if (m_product == 4)
    {
        m_pending = frame.m_kind == NI_REQUEST;
        if (m_pending)
        {
            m_outer = frame;
            m_outer.m_payload = nullptr;
            m_origin = origin;
            m_accountChild = next.m_requestId;
        }
    }
    return true;
}

void NetServiceTestProbe::onServiceClosed(int32_t reason)
{
    (void)reason;
    m_bad = m_bad || m_closed;
    m_closed = true;
}

NetServiceTestHost::NetServiceTestHost(uint32_t product, Semaphore& ready) : m_probe(product, ready), m_service(m_inbox)
{
    m_probe.m_service = &m_service;
    ProcessWork& work = ProcessWork::instance();
    work.addProcessReceiveCallback(&m_inbox);
    if (!work.initReceive(16452, 16))
    {
        work.uninitReceive();
        throw std::runtime_error("service test IPC init");
    }
}

NetServiceTestHost::~NetServiceTestHost()
{
    m_service.shutdown();
    ProcessWork::instance().uninitReceive();
}

int NetServiceTestCases::childMain(int32_t parent, uint32_t product, uint32_t mode)
{
    Semaphore ready;
    Semaphore stop;
    CheckService(ready.tryOpenProcessSemaphore(ServiceSignalName("ready", parent, product, mode)) &&
        stop.tryOpenProcessSemaphore(ServiceSignalName("stop", parent, product, mode)), "service test signals");
    NetServiceTestHost host(product, ready);
    NetGatewayServiceCredential credential = TestCredential(product);
    if (mode == 2)
    {
        credential.m_secret[0] ^= 0xff;
    }
    CheckService(host.m_service.connect(parent, NetInterfaceTlsTestPair::options(false, "localhost"), product,
        credential.m_secret, &host.m_probe) == NI_OK, "service test connect");
    const auto start = std::chrono::steady_clock::now();
    while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() < 9000)
    {
        host.m_service.poll();
        if (host.m_probe.m_bad)
        {
            return 41;
        }
        if (host.m_probe.m_closed)
        {
            return mode != 1 && !host.m_probe.m_ready ? 0 : 42;
        }
        if (stop.processWait(0))
        {
            return mode == 1 && host.m_probe.m_ready && !host.m_probe.m_pending ? 0 : 43;
        }
        Sleep(1);
    }
    return 44;
}

NetServiceTestChild::NetServiceTestChild(uint32_t product, uint32_t mode) : m_process(nullptr), m_ready(false)
{
    const int32_t pid = CSystem::currentProcessPid();
    m_readySignal.createProcessSemaphore(ServiceSignalName("ready", pid, product, mode), 1);
    m_stopSignal.createProcessSemaphore(ServiceSignalName("stop", pid, product, mode), 1);
    wchar_t path[32768] = {};
    const DWORD length = GetModuleFileNameW(nullptr, path, 32768);
    CheckService(length != 0 && length < 32768, "service test executable path");
    std::wostringstream args;
    args << L'"' << path << L"\" --service-child=" << pid << L" --service-product=" << product <<
        L" --service-mode=" << mode;
    std::wstring command = args.str();
    STARTUPINFOW startup = {};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION child = {};
    CheckService(CreateProcessW(path, &command[0], nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
        nullptr, nullptr, &startup, &child) != FALSE, "service test child create");
    CloseHandle(child.hThread);
    m_process = child.hProcess;
}

NetServiceTestChild::~NetServiceTestChild()
{
    if (m_process != nullptr)
    {
        m_stopSignal.tryProcessSignal();
        if (WaitForSingleObject(m_process, 2000) == WAIT_TIMEOUT)
        {
            TerminateProcess(m_process, 45);
            WaitForSingleObject(m_process, 2000);
        }
        CloseHandle(m_process);
    }
}

bool NetServiceTestChild::ready()
{
    m_ready = m_ready || m_readySignal.processWait(0);
    return m_ready;
}

uint32_t NetServiceTestChild::exitCode() const
{
    DWORD code = 0;
    CheckService(GetExitCodeProcess(m_process, &code) != FALSE, "service test child status");
    return code;
}

uint32_t NetServiceTestChild::stop()
{
    CheckService(m_stopSignal.tryProcessSignal() == 0, "service test stop signal");
    CheckService(WaitForSingleObject(m_process, 2000) == WAIT_OBJECT_0, "service test stop timeout");
    return exitCode();
}

/** 启用隔离网关和两份公开测试凭据
@param [in,out] runtime 本例网关
*/
static void StartServiceGateway(NetGatewayLocalServer& runtime)
{
    std::vector<NetGatewayServiceCredential> credentials;
    credentials.push_back(TestCredential(0));
    credentials.push_back(TestCredential(4));
    const NetInterfaceTlsOptions tls = NetInterfaceTlsTestPair::options(true, "localhost");
    CheckService(runtime.startLocal(tls, credentials) == NI_OK &&
        runtime.gateway().listen("127.0.0.1", 0, tls) == NI_OK, "service gateway start");
}

void NetServiceTestCases::nestedProcesses()
{
    NetGatewayLocalServer runtime;
    StartServiceGateway(runtime);
    NetServiceTestChild account(0, 1);
    NetServiceTestChild product(4, 1);
    NetGatewayTestClient client;
    NetInterfaceTlsOptions tls = NetInterfaceTlsTestPair::options(false, "localhost");
    CheckService(client.m_network.connect("127.0.0.1", runtime.gateway().localPort(), tls, &client, client.m_peer) == NI_OK,
        "service public client connect");
    while (!account.ready() || !product.ready() || !client.m_applicationReady)
    {
        netTestCheckpoint();
        runtime.poll();
        client.poll();
        CheckService(account.exitCode() == STILL_ACTIVE && product.exitCode() == STILL_ACTIVE, "service startup child exited");
        Sleep(1);
    }
    CheckService(runtime.serviceCount() == 2, "service registration count");
    std::vector<uint8_t> payload(1048560);
    for (uint32_t index = 0; index < payload.size(); ++index)
    {
        payload[index] = static_cast<uint8_t>(index * 17 + 193);
    }
    std::vector<uint8_t> bytes(payload.size() + 16);
    uint32_t size = 0;
    CheckService(NetInterfaceRpc::encode(NI_REQUEST, 8000, 0, payload.data(), static_cast<uint32_t>(payload.size()),
        bytes.data(), static_cast<uint32_t>(bytes.size()), size) == NI_OK, "service root RPC encode");
    NetInterfaceFrame request = {};
    request.m_kind = NI_REQUEST;
    request.m_requestId = UINT64_C(0xfedcba9876543210);
    request.m_messageType = 700;
    request.m_productId = 4;
    request.m_payload = bytes.data();
    request.m_payloadSize = size;
    CheckService(client.m_network.send(client.m_peer, request) == NI_OK, "service root queue");
    while (client.m_received.empty())
    {
        netTestCheckpoint();
        runtime.poll();
        client.poll();
        CheckService(account.exitCode() == STILL_ACTIVE && product.exitCode() == STILL_ACTIVE, "service routing child exited");
        Sleep(1);
    }
    NetInterfaceFrame response = client.m_received[0].m_frame;
    response.m_payload = client.m_received[0].m_payload.data();
    NetInterfaceRpcView rpc = {};
    CheckService(response.m_requestId == request.m_requestId && response.m_productId == 4 && response.m_messageType == 700 &&
        NetInterfaceRpc::parse(response, rpc) == NI_OK && rpc.m_value == NI_RPC_SUCCESS &&
        rpc.m_size == payload.size() && std::memcmp(rpc.m_data, payload.data(), payload.size()) == 0 &&
        runtime.gateway().pendingCount() == 0, "service full A/B IPC chain did not preserve response");
    CheckService(product.stop() == 0 && account.stop() == 0, "service child validation failed");
    client.m_network.shutdown();
    runtime.shutdown();
}

void NetServiceTestCases::rejectedServices()
{
    NetGatewayLocalServer runtime;
    StartServiceGateway(runtime);
    NetServiceTestChild product(4, 1);
    while (!product.ready())
    {
        netTestCheckpoint();
        runtime.poll();
        CheckService(product.exitCode() == STILL_ACTIVE, "authorized service startup failed");
        Sleep(1);
    }
    NetServiceTestChild wrong(0, 2);
    NetServiceTestChild duplicate(4, 3);
    while (wrong.exitCode() == STILL_ACTIVE || duplicate.exitCode() == STILL_ACTIVE)
    {
        netTestCheckpoint();
        runtime.poll();
        Sleep(1);
    }
    CheckService(wrong.exitCode() == 0 && duplicate.exitCode() == 0 && runtime.serviceCount() == 1 &&
        product.exitCode() == STILL_ACTIVE, "rejected service registered or replaced live product");
    CheckService(product.stop() == 0, "original service lost authentication");
    runtime.shutdown();
}

int NetServiceTestCases::standaloneMain(int32_t gatewayPid, uint32_t product)
{
    Semaphore ready;
    ready.createProcessSemaphore(ServiceSignalName("standalone", CSystem::currentProcessPid(), product, 4), 1);
    NetServiceTestHost host(product, ready);
    host.m_probe.m_expectedPlatform = 1;
    const auto credential = TestCredential(product);
    auto tls = NetInterfaceTlsTestPair::options(false, "localhost");
    CheckService(host.m_service.connect(gatewayPid, tls, product, credential.m_secret, &host.m_probe) == NI_OK,
        "standalone service connect");
    const auto start = std::chrono::steady_clock::now();
    bool announced = false;
    while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() < 10000)
    {
        host.m_service.poll();
        if (host.m_probe.m_bad || host.m_probe.m_closed) return 51;
        if (!announced && host.m_probe.m_ready)
        {
            std::printf("TEST_SERVICE_READY product=%u\n", product);
            std::fflush(stdout);
            announced = true;
        }
        Sleep(1);
    }
    return host.m_probe.m_ready && !host.m_probe.m_pending ? 0 : 52;
}

NetDesktopClientProbe::NetDesktopClientProbe() : m_sender(nullptr), m_id(0), m_results(0), m_closes(0), m_bad(false)
{

}

void NetDesktopClientProbe::onSessionReady(uint64_t generation, uint32_t maxRequests, uint32_t maxChunk)
{
    std::printf("DESKTOP_READY generation=%llu limit=%u chunk=%u\n", generation, maxRequests, maxChunk);
    const uint8_t bytes[] = {0, 17, 0, 255, 42, 97, 98, 99};
    m_bad = generation != 1 || maxRequests != 4 || maxChunk != 4096 ||
        m_sender->beginRequest(4, 700, 7000, m_id) != NI_OK ||
        m_sender->postRequest(m_id, bytes, sizeof(bytes)) != NI_OK;
}

void NetDesktopClientProbe::onSessionResult(uint64_t generation, uint64_t requestId, uint32_t product,
    uint32_t type, uint32_t status, const uint8_t* data, uint32_t size)
{
    std::printf("DESKTOP_RPC generation=%llu id=%llu product=%u type=%u status=%u size=%u\n",
        generation, requestId, product, type, status, size);
    const uint8_t expected[] = {0, 17, 0, 255, 42, 97, 98, 99};
    m_bad = m_bad || generation != 1 || requestId != m_id || product != 4 || type != 700 ||
        status != NI_RPC_SUCCESS || size != sizeof(expected) || data == nullptr ||
        (size == sizeof(expected) && std::memcmp(data, expected, sizeof(expected)) != 0);
    ++m_results;
}

bool NetDesktopClientProbe::onSessionFrame(uint64_t generation, const NetInterfaceFrame& frame)
{
    (void)generation;
    (void)frame;
    m_bad = true;
    return true;
}

void NetDesktopClientProbe::onSessionClosed(uint64_t generation, int32_t reason, int32_t detail)
{
    std::printf("DESKTOP_CLOSED generation=%llu reason=%d detail=%d\n", generation, reason, detail);
    (void)detail;
    m_bad = m_bad || generation != 1 || reason != 0;
    ++m_closes;
}

int NetServiceTestCases::desktopClientMain(int32_t gatewayPid)
{
    NetDesktopClientProbe events;
    NetSenderTestClient<NetSender, NetSenderClient>& sender = NetSenderTestClient<NetSender, NetSenderClient>::instance();
    events.m_sender = &sender;
    if (sender.prepareGateway() != NI_OK) return 53;
    int result = 54;
    try
    {
        NetSenderGatewayOptions options = {};
        options.m_network.structSize = sizeof(NetWorkClientOptions);
        options.m_network.version = 1;
        options.m_network.platform = 1;
        options.m_network.capabilities = 5;
        options.m_network.maxRequests = 4;
        options.m_network.maxChunk = 4096;
        options.m_network.handshakeTimeoutMs = 5000;
        options.m_localGatewayPid = 0;
        if (NetSenderDesktopBackend::configureGateway(sender, gatewayPid) != NI_OK)
        {
            throw std::runtime_error("desktop gateway backend configuration");
        }
        std::memset(options.m_network.device, 0x42, 16);
        auto tls = NetInterfaceTlsTestPair::options(false, "localhost");
        tls.entropy = nullptr;
        tls.entropyContext = nullptr;
        options.m_network.tls = tls;
        CheckService(sender.connectGateway(options, &events) == NI_OK, "desktop NetSenderTestClient<NetSender, NetSenderClient> gateway connect");
        const auto start = std::chrono::steady_clock::now();
        while (events.m_results == 0 && !events.m_bad && events.m_closes == 0 &&
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() < 9000)
        {
            sender.pollGateway();
            Sleep(1);
        }
        sender.closeGateway(0);
        while (events.m_closes == 0 &&
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() < 9500)
        {
            sender.pollGateway();
            Sleep(1);
        }
        sender.uninitGateway();
        if (!events.m_bad && events.m_results == 1 && events.m_closes == 1) result = 0;
        std::printf("DESKTOP_CLIENT_RESULT code=%d results=%u closes=%u bad=%u\n", result,
            events.m_results, events.m_closes, events.m_bad ? 1U : 0U);
        std::fflush(stdout);
    }
    catch (...)
    {
        sender.uninitGateway();
    }
    while (sender.pollGateway()) { CSystem::sleepPrecise(1); }
    sender.releaseGateway();

    return result;
}

#endif