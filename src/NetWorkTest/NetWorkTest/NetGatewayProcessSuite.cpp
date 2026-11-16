#include "NetGatewayProcessSuite.h"
#include "NetGatewayWindowsProbe.h"
#include "NetInterface/NetInterfaceAPI.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <sstream>
#include <stdexcept>
#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#endif

/** 写入本次测试文件，并把写入或关闭失败报告给测试，而非默认成功
@param [in] path 本次独占目录内路径
@param [in] text 完整UTF8文本
@return 完整写入且关闭成功返回true
*/
static bool ProcessSuiteWrite(const std::string& path, const std::string& text)
{
    intptr_t handle = CSystem::openFileHandle(path.c_str(), 2);
    if (handle == -1)
    {
        return false;
    }
    const bool written = CSystem::writeFileHandle(handle, 0, reinterpret_cast<const uint8_t*>(text.data()),
        static_cast<uint32_t>(text.size()));
    const bool closed = CSystem::closeFileHandle(&handle);
    return written && closed;
}

NetGatewayProcessSuite::NetGatewayProcessSuite(const std::string& binaries, const std::string& output) :
m_binaries(binaries),
m_output(output)
{

}

NetGatewayProcessSuite::~NetGatewayProcessSuite()
{
    // 析构路径不抛异常；失败信息由run的最终退出检查负责记录。
    try
    {
        for (size_t index = 0; index < m_residents.size(); ++index)
        {
            ProcessSuiteWrite(m_output + "/" + m_residents[index] + ".stop", "stop");
        }
        const std::chrono::steady_clock::time_point end =
            std::chrono::steady_clock::now() + std::chrono::seconds(5);
        for (size_t index = m_children.size(); index != 0; --index)
        {
            const int64_t remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
                end - std::chrono::steady_clock::now()).count();
            int32_t code = 0;
            m_children[index - 1]->wait(static_cast<uint32_t>((std::max<int64_t>)(0, remaining)), code);
        }
    }
    catch (...)
    {
        // unique_ptr继续回收每个拥有的进程及其Windows作业，不按名字扫描系统进程。
    }
}

bool NetGatewayProcessSuite::leasesFree()
{
#ifdef _WIN32
    const wchar_t* names[] = { L"Global\\xyls.NetClientManager.owner",
        L"Global\\xyls.NetServerManager.owner", L"Global\\xyls.NetClientManagerSupervisor.owner" };
    for (size_t index = 0; index < 3; ++index)
    {
        HANDLE lease = OpenMutexW(SYNCHRONIZE | MUTEX_MODIFY_STATE, FALSE, names[index]);
        if (lease == nullptr)
        {
            if (GetLastError() != ERROR_FILE_NOT_FOUND)
            {
                return false;
            }
            continue;
        }
        const DWORD state = WaitForSingleObject(lease, 0);
        bool free = state == WAIT_OBJECT_0 || state == WAIT_ABANDONED;
        if (free)
        {
            free = ReleaseMutex(lease) != FALSE;
        }
        CloseHandle(lease);
        if (!free)
        {
            return false;
        }
    }
    return true;
#else
    return false;
#endif
}

NetTestProcess& NetGatewayProcessSuite::launch(const std::string& name, const std::string& filename,
    const std::vector<std::string>& arguments, bool resident, bool normalStartup)
{
    std::vector<std::string> values(arguments);
    if (resident)
    {
        values.push_back("--stop-file=" + m_output + "/" + name + ".stop");
        values.push_back("--run-for-ms=90000");
        m_residents.push_back(name);
    }
    return launchPath(name, m_binaries + "/" + filename, values, normalStartup);
}

NetTestProcess& NetGatewayProcessSuite::launchPath(const std::string& name, const std::string& executable,
    const std::vector<std::string>& arguments, bool normalStartup)
{
    std::unique_ptr<NetTestProcess> process(new NetTestProcess());
    if (!process->start(executable, arguments, m_output + "/" + name + ".log", normalStartup))
    {
        throw std::runtime_error("Cannot start owned fixture: " + name);
    }
    NetTestProcess& reference = *process;
    m_children.push_back(std::move(process));
    return reference;
}

std::string NetGatewayProcessSuite::waitLine(NetTestProcess& process, const std::string& prefix, uint32_t timeoutMs)
{
    const std::chrono::steady_clock::time_point end =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    do
    {
        std::string text;
        if (!process.output(text))
        {
            throw std::runtime_error("Cannot read owned process output");
        }
        size_t begin = 0;
        for (size_t newline = text.find('\n'); newline != std::string::npos; newline = text.find('\n', begin))
        {
            std::string line = text.substr(begin, newline - begin);
            if (!line.empty() && line.back() == '\r')
            {
                line.pop_back();
            }
            if (line.compare(0, prefix.size(), prefix) == 0)
            {
                return line;
            }
            begin = newline + 1;
        }
        int32_t code = 0;
        if (process.poll(code) != 0)
        {
            throw std::runtime_error("Process stopped before marker: " + prefix);
        }
        CSystem::Sleep(1);
    } while (std::chrono::steady_clock::now() < end);
    throw std::runtime_error("Process marker deadline: " + prefix);
}

uint32_t NetGatewayProcessSuite::number(const std::string& line, const std::string& key)
{
    std::istringstream fields(line);
    std::string field;
    uint32_t value = 0;
    bool found = false;
    while (fields >> field)
    {
        if (field.compare(0, key.size(), key) != 0)
        {
            continue;
        }
        if (found || field.size() == key.size())
        {
            throw std::runtime_error("Duplicate or missing numeric marker: " + key);
        }
        found = true;
        for (size_t index = key.size(); index < field.size(); ++index)
        {
            const char byte = field[index];
            if (byte < '0' || byte > '9' || value > (UINT32_MAX - (byte - '0')) / 10)
            {
                throw std::runtime_error("Invalid numeric marker: " + key);
            }
            value = value * 10 + byte - '0';
        }
    }
    if (!found || value == 0)
    {
        throw std::runtime_error("Absent numeric marker: " + key);
    }
    return value;
}

void NetGatewayProcessSuite::exited(NetTestProcess& process, uint32_t timeoutMs)
{
    int32_t code = 0;
    const int32_t state = process.wait(timeoutMs, code);
    if (state != 1 || code != 0)
    {
        throw std::runtime_error("Owned process did not exit normally: pid=" + std::to_string(process.pid()) +
            " state=" + std::to_string(state) + " exit=" + std::to_string(code));
    }
}

void NetGatewayProcessSuite::stop(const std::string& name)
{
    if (std::find(m_residents.begin(), m_residents.end(), name) == m_residents.end() ||
        !ProcessSuiteWrite(m_output + "/" + name + ".stop", "stop"))
    {
        throw std::runtime_error("Cannot stop owned resident: " + name);
    }
}

NetTestProcess& NetGatewayProcessSuite::server(const std::string& name, uint32_t& port)
{
    NetTestProcess& process = launch(name, "NetServerManager1.1.exe", {
        "--listen=127.0.0.1", "--advertise=127.0.0.1", "--port=" + std::to_string(port),
        "--identity-directory=" + m_output + "/identity" }, true);
    const std::string line = waitLine(process, "GATEWAY_READY ", 12000);
    if (number(line, "pid=") != process.pid())
    {
        throw std::runtime_error("Server readiness belongs to a different process");
    }
    const uint32_t actual = number(line, "port=");
    if (actual > 65535 || (port != 0 && port != actual))
    {
        throw std::runtime_error("Replacement server did not bind the requested port");
    }
    port = actual;
    return process;
}

NetTestProcess& NetGatewayProcessSuite::service(const std::string& name, uint32_t gateway)
{
    NetTestProcess& process = launch(name, "NetWorkTest1.0.exe", { "--self-test-service=" + std::to_string(gateway) }, true);
    if (number(waitLine(process, "SERVICE_PROBE_READY ", 12000), "pid=") != process.pid())
    {
        throw std::runtime_error("Service readiness belongs to a different process");
    }
    return process;
}

NetTestProcess& NetGatewayProcessSuite::client(const std::string& name, uint32_t port)
{
    NetTestProcess& process = launch(name, "NetClientManager1.1.exe", { "--address=127.0.0.1",
        "--port=" + std::to_string(port), "--cache-file=" + m_output + "/client-nodes.cache" }, true);
    if (number(waitLine(process, "CLIENT_GATEWAY_READY ", 12000), "pid=") != process.pid())
    {
        throw std::runtime_error("Client readiness belongs to a different process");
    }
    return process;
}

void NetGatewayProcessSuite::replacement()
{
    uint32_t port = 0;
    NetTestProcess& firstServer = server("server-a", port);
    NetTestProcess& firstService = service("service-a", firstServer.pid());
    NetTestProcess& firstClient = client("desktop-a", port);
    NetTestProcess& desktop = launch("desktop-endpoint", "NetWorkTest1.0.exe", { "--recovery-desktop=0" }, false);
    waitLine(desktop, "RECOVERY_ROUND count=1 ", 12000);
    stop("desktop-a");
    exited(firstClient, 5000);
    NetTestProcess& nextClient = client("desktop-b", port);
    exited(desktop, 15000);
    waitLine(desktop, "RECOVERY_CLIENT_PASS desktop=1 ", 1);
    NetTestProcess& native = launch("native-endpoint", "NetWorkTest1.0.exe",
        { "--recovery-native=127.0.0.1:" + std::to_string(port) }, false);
    waitLine(native, "RECOVERY_ROUND count=1 ", 12000);
    stop("server-a");
    exited(firstServer, 5000);
    // Link结束输入归一为TRUNCATED，IPC死亡探测可给INVALID_STATE；只接受这两种断连终态。
    int32_t serviceCode = 0;
    std::string serviceLog;
    if (firstService.wait(7000, serviceCode) != 1 || !firstService.output(serviceLog))
    {
        throw std::runtime_error("Service did not exit after its gateway stopped");
    }
    serviceLog.erase(std::remove(serviceLog.begin(), serviceLog.end(), '\r'), serviceLog.end());
    if (serviceLog.find("SERVICE_PROBE_REJECT") != std::string::npos ||
        (serviceCode != 0 && (serviceCode != 1 ||
            (serviceLog.find("SERVICE_PROBE_CLOSED reason=-7\n") == std::string::npos &&
                serviceLog.find("SERVICE_PROBE_CLOSED reason=-6\n") == std::string::npos))))
    {
        throw std::runtime_error("Service exited without the expected gateway-loss notification");
    }
    NetTestProcess& nextServer = server("server-b", port);
    NetTestProcess& nextService = service("service-b", nextServer.pid());
    exited(native, 15000);
    waitLine(native, "RECOVERY_CLIENT_PASS desktop=0 ", 1);
    stop("desktop-b");
    exited(nextClient, 5000);
    // 业务先有序退出，再关其网关；同时置停止文件会把收尾变成一次未声明的断链注入。
    stop("service-b");
    exited(nextService, 5000);
    stop("server-b");
    exited(nextServer, 5000);
}

uint32_t NetGatewayProcessSuite::recovered(NetTestProcess& root, uint32_t previous, uint32_t guard)
{
    const std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    do
    {
        std::vector<uint32_t> clients;
        std::vector<uint32_t> guards;
        if (!root.ownedProcesses(m_binaries + "/NetClientManager1.1.exe", clients) ||
            !root.ownedProcesses(m_binaries + "/NetClientManagerGuard1.1.exe", guards) ||
            guards.size() != 1 || guards[0] != guard || clients.size() > 1)
        {
            throw std::runtime_error("Recovery lost its owned guard or created multiple gateways");
        }
        if (clients.size() == 1 && clients[0] != previous &&
            CSystem::isProcessLeaseReady("NetClientManager", clients[0]))
        {
            return clients[0];
        }
        CSystem::Sleep(10);
    } while (std::chrono::steady_clock::now() < end);
    throw std::runtime_error("Owned guard did not recover the gateway");
}

void NetGatewayProcessSuite::descendantsStopped(NetTestProcess& root, const std::string& filename, uint32_t timeoutMs)
{
    const std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    do
    {
        std::vector<uint32_t> processes;
        if (!root.ownedProcesses(m_binaries + "/" + filename, processes))
        {
            throw std::runtime_error("Cannot inspect owned process tree: " + filename);
        }
        if (processes.empty()) { return; }
        CSystem::Sleep(10);
    } while (std::chrono::steady_clock::now() < end);
    throw std::runtime_error("Owned descendant did not stop: " + filename);
}

void NetGatewayProcessSuite::singleton()
{
#ifdef _WIN32
    // 默认服务端绑定5203，先用独占socket证明无既有监听者；失败即停止夹具。
    WSADATA data = {};
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) { throw std::runtime_error("WSAStartup failed"); }
    SOCKET socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    BOOL exclusive = TRUE;
    sockaddr_in address = {};
    address.sin_family = AF_INET;
    address.sin_port = htons(5203);
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    const bool available = socket != INVALID_SOCKET &&
        setsockopt(socket, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, reinterpret_cast<const char*>(&exclusive), sizeof(exclusive)) == 0 &&
        bind(socket, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) == 0;
    if (socket != INVALID_SOCKET) { closesocket(socket); }
    WSACleanup();
    if (!available) { throw std::runtime_error("Default port 5203 is occupied; no existing listener was changed"); }
    for (uint32_t role = 0; role < 2; ++role)
    {
        const bool clientRole = role == 1;
        const std::string name = clientRole ? "NetClientManager" : "NetServerManager";
        const std::string executable = name + "1.1.exe";
        const std::string ready = clientRole ? "CLIENT_GATEWAY_READY " : "GATEWAY_READY ";
        std::vector<std::string> options;
        if (clientRole)
        {
            options = { "--address=127.0.0.1", "--port=5203", "--cache-file=" + m_output + "/singleton.cache" };
        }
        else
        {
            options = { "--listen=127.0.0.1", "--advertise=127.0.0.1", "--port=0",
                "--identity-directory=" + m_output + "/singleton-identity" };
        }
        NetTestProcess& first = launch(name + "-old", executable, options, true);
        if (number(waitLine(first, ready, 12000), "pid=") != first.pid())
        {
            throw std::runtime_error("Singleton readiness PID mismatch");
        }
        std::vector<NetTestProcess*> reuse;
        // 先启动全部八个进程，再逐个等待，不能顺序启动/等待冒充并发复用。
        for (uint32_t index = 0; index < 8; ++index)
        {
            reuse.push_back(&launch(name + "-reuse-" + std::to_string(index), executable,
                clientRole ? std::vector<std::string>{ "127.0.0.1" } : std::vector<std::string>{ "--port=5204" }, false));
        }
        for (size_t index = 0; index < reuse.size(); ++index)
        {
            exited(*reuse[index], 5000);
            waitLine(*reuse[index], name + "_REUSED", 1);
        }
        int32_t code = 0;
        if (first.poll(code) != 0 || !CSystem::isProcessLeaseReady(name, first.pid()))
        {
            throw std::runtime_error("Concurrent reuse replaced or stopped the resident");
        }
        if (clientRole)
        {
            NetTestProcess& plain = launch(name + "-no-argument", executable, {}, false);
            exited(plain, 5000);
            waitLine(plain, name + "_REUSED", 1);
            if (first.poll(code) != 0) { throw std::runtime_error("No-argument client stopped the resident"); }
        }
        std::vector<std::string> replacementOptions;
        if (clientRole)
        {
            replacementOptions = options;
            replacementOptions.push_back("--replace");
        }
        NetTestProcess& next = launch(name + "-replacement", executable, replacementOptions, clientRole);
        exited(first, 12000);
        if (number(waitLine(next, ready, 12000), "pid=") != next.pid() || next.pid() == first.pid())
        {
            throw std::runtime_error("Replacement did not publish its own new PID");
        }
        uint32_t guard = 0;
        if (clientRole)
        {
            guard = number(waitLine(next, "CLIENT_GATEWAY_WATCHDOG ", 1000), "pid=");
            descendantsStopped(first, "NetClientManagerGuard1.1.exe", 4000);
        }
        if (!next.terminate()) { throw std::runtime_error("Could not simulate an owned gateway crash"); }
        if (clientRole)
        {
            NetTestProcess& duringBackoff = launch(name + "-backoff", executable, options, false);
            exited(duringBackoff, 5000);
            waitLine(duringBackoff, name + "_REUSED", 1);
            const uint32_t restored = recovered(next, next.pid(), guard);
            if (restored == duringBackoff.pid()) { throw std::runtime_error("Reuse process became the replacement"); }
            stop(name + "-replacement");
            descendantsStopped(next, executable, 5000);
            descendantsStopped(next, "NetClientManagerGuard1.1.exe", 5000);
        }
        else
        {
            NetTestProcess& restarted = launch(name + "-restart", executable, options, true);
            if (number(waitLine(restarted, ready, 12000), "pid=") != restarted.pid())
            {
                throw std::runtime_error("Crash left a stale singleton lease");
            }
            stop(name + "-restart");
            exited(restarted, 5000);
        }
        std::printf("SINGLETON_ROLE name=%s parallelReuses=8 defaultStartup=verified replacement=verified crash=verified stop=verified PASS\n",
            name.c_str());
        std::fflush(stdout);
    }
#else
    throw std::runtime_error("Singleton Windows fixture unavailable on this platform");
#endif
}

void NetGatewayProcessSuite::background()
{
#ifdef _WIN32
    const std::string executable = m_binaries + "/NetClientManager1.1.exe";
    const std::string guardExecutable = m_binaries + "/NetClientManagerGuard1.1.exe";
    NetGatewayWindowsProbe windows;
    if (!NetGatewayWindowsProbe::guiImage(executable) || !NetGatewayWindowsProbe::guiImage(guardExecutable) ||
        NetGatewayWindowsProbe::guiImage(m_binaries + "/NetWorkTest1.0.exe"))
    {
        throw std::runtime_error("GUI subsystem positive/console negative check failed");
    }
    uint32_t port = 0;
    NetTestProcess& gatewayServer = server("background-server", port);
    NetTestProcess& businessService = service("background-service", gatewayServer.pid());
    NetTestProcess& gateway = launch("background-client", "NetClientManager1.1.exe",
        { "--background", "--address=127.0.0.1", "--port=" + std::to_string(port),
            "--cache-file=" + m_output + "/background.cache" }, true, true);
    if (number(waitLine(gateway, "CLIENT_GATEWAY_READY ", 12000), "pid=") != gateway.pid())
    {
        throw std::runtime_error("Background gateway readiness PID mismatch");
    }
    const uint32_t guard = number(waitLine(gateway, "CLIENT_GATEWAY_WATCHDOG ", 1000), "pid=");
    windows.window(gateway, executable, gateway.pid());
    windows.window(gateway, guardExecutable, guard);
    if (guard == gateway.pid() || CSystem::isProcessLeaseReady("NetClientManager", guard) ||
        NetGatewayWindowsProbe::receiver("NetClientManager1.1") != gateway.pid() ||
        NetGatewayWindowsProbe::receiver("NetClientManagerGuard1.1") != 0)
    {
        throw std::runtime_error("Guard reused the gateway identity or IPC receiver name");
    }
    NetTestProcess& console = launch("background-console", "NetWorkTest1.0.exe",
        { "--test-process-probe", "console", std::to_string(gateway.pid()) }, false);
    exited(console, 5000);
    std::vector<NetTestProcess*> reuse;
    for (uint32_t index = 0; index < 8; ++index)
    {
        reuse.push_back(&launch("background-reuse-" + std::to_string(index), "NetClientManager1.1.exe",
            index % 2 == 0 ? std::vector<std::string>() : std::vector<std::string>{ "--background" }, false, true));
    }
    for (size_t index = 0; index < reuse.size(); ++index)
    {
        exited(*reuse[index], 5000);
        waitLine(*reuse[index], "NetClientManager_REUSED", 1);
    }
    int32_t code = 0;
    for (uint32_t index = 0; index < 2; ++index)
    {
        NetTestProcess& business = launch("background-business-" + std::to_string(index), "NetWorkTest1.0.exe",
            { "--facade-service-client=0" }, false);
        exited(business, 12000);
        waitLine(business, "CPP_SENDER_CLIENT_PASS", 1);
        if (gateway.poll(code) != 0 || !CSystem::isProcessLeaseReady("NetClientManager", gateway.pid()))
        {
            throw std::runtime_error("Business exit stopped the resident gateway");
        }
    }
    std::puts("BACKGROUND gui=verified names=verified parallelReuse=8 independentBusiness=2 PASS");
    NetTestProcess& endpoint = launch("background-recovery", "NetWorkTest1.0.exe", { "--recovery-desktop=0" }, false);
    const uint32_t generation = number(waitLine(endpoint, "RECOVERY_ROUND count=1 ", 12000), "generation=");
    if (!gateway.terminate()) { throw std::runtime_error("Cannot inject first owned gateway crash"); }
    // 立即启动才能覆盖退避窗口；先等closed可能已错过守护的首次退避。
    NetTestProcess& backoff = launch("background-backoff", "NetWorkTest1.0.exe", { "--client-launch-probe" }, false);
    exited(backoff, 6000);
    const std::string launchLine = waitLine(backoff, "CLIENT_LAUNCH ", 1);
    const std::string successPrefix = "CLIENT_LAUNCH result=0 pid=";
    if (launchLine.compare(0, successPrefix.size(), successPrefix) != 0)
    {
        throw std::runtime_error("SDK launch during recovery did not reuse a live gateway or guard");
    }
    const uint32_t selected = number(launchLine, "pid=");
    waitLine(endpoint, "RECOVERY_CLOSED ", 7000);
    const uint32_t newerGeneration = number(waitLine(endpoint, "RECOVERY_ROUND count=2 ", 15000), "generation=");
    exited(endpoint, 5000);
    waitLine(endpoint, "RECOVERY_CLIENT_PASS desktop=1 ", 1);
    const uint32_t recoveredPid = recovered(gateway, gateway.pid(), guard);
    if (newerGeneration <= generation || (selected != guard && selected != recoveredPid) ||
        NetGatewayWindowsProbe::receiver("NetClientManager1.1") != recoveredPid ||
        NetGatewayWindowsProbe::receiver("NetClientManagerGuard1.1") != 0)
    {
        throw std::runtime_error("Recovery identity, request generation or SDK selection changed incorrectly");
    }
    if (!gateway.terminateOwned(recoveredPid, executable))
    {
        throw std::runtime_error("Cannot inject second owned gateway crash");
    }
    const uint32_t againPid = recovered(gateway, recoveredPid, guard);
    NetTestProcess& again = launch("background-business-again", "NetWorkTest1.0.exe", { "--facade-service-client=0" }, false);
    exited(again, 12000);
    waitLine(again, "CPP_SENDER_CLIENT_PASS", 1);
    std::puts("BACKGROUND crashes=2 sameGuard=verified recovered65537ByteRoundTrip=verified PASS");
    HWND window = windows.window(gateway, executable, againPid);
    if (NetGatewayWindowsProbe::message(window, againPid, WM_QUERYENDSESSION) != TRUE)
    {
        throw std::runtime_error("Gateway rejected the session end query");
    }
    NetGatewayWindowsProbe::message(window, againPid, WM_ENDSESSION, FALSE);
    if (!CSystem::isProcessLeaseReady("NetClientManager", againPid))
    {
        throw std::runtime_error("Canceled session end stopped the gateway");
    }
    NetGatewayWindowsProbe::message(window, againPid, WM_ENDSESSION, TRUE);
    descendantsStopped(gateway, "NetClientManager1.1.exe", 7000);
    descendantsStopped(gateway, "NetClientManagerGuard1.1.exe", 7000);
    CSystem::Sleep(1200);
    descendantsStopped(gateway, "NetClientManager1.1.exe", 1);
    descendantsStopped(gateway, "NetClientManagerGuard1.1.exe", 1);
    NetTestProcess& wrongRole = launch("background-wrong-role", "NetClientManager1.1.exe", { "--watch-parent=1" }, false);
    if (wrongRole.wait(5000, code) != 1 || code != 2) { throw std::runtime_error("Gateway accepted the guard entry"); }
    const std::vector<std::vector<std::string>> invalid = { {}, { "--background" }, { "--watch-parent=0" }, { "--watch-parent=abc" } };
    for (size_t index = 0; index < invalid.size(); ++index)
    {
        NetTestProcess& rejected = launch("background-invalid-guard-" + std::to_string(index),
            "NetClientManagerGuard1.1.exe", invalid[index], false);
        if (rejected.wait(5000, code) != 1 || code != 2) { throw std::runtime_error("Guard accepted a business launch"); }
    }
    deployments(port);
    startup();
    stop("background-client");
    stop("background-service");
    exited(businessService, 5000);
    stop("background-server");
    exited(gatewayServer, 5000);
    std::puts("BACKGROUND sessionEnd=verified invalidRoles=5 deploymentLayouts=3 missingGuard=verified startup=verified PASS");
#else
    throw std::runtime_error("Background Windows fixture unavailable on this platform");
#endif
}

void NetGatewayProcessSuite::deployments(uint32_t port)
{
#ifdef _WIN32
    const std::string executable = m_binaries + "/NetClientManager1.1.exe";
    const std::string guardExecutable = m_binaries + "/NetClientManagerGuard1.1.exe";
    NetGatewayWindowsProbe windows;
    for (uint32_t layout = 0; layout < 3; ++layout)
    {
        // UTF8字节常量不受VS2013执行字符集影响，路径含中文和真实空格。
        const std::string root = m_output + "/deployment-" + std::to_string(layout) +
            "-\xe4\xb8\xad\xe6\x96\x87 \xe7\xa9\xba\xe6\xa0\xbc";
        const std::string base = layout == 0 ? root + "/Business" : root;
        const std::string targetDirectory = layout == 0 ? root + "/NetClientManager" : root;
        const std::string target = targetDirectory + "/NetClientManager1.1.exe";
        const std::string targetGuard = targetDirectory + "/NetClientManagerGuard1.1.exe";
        const std::string businessPath = base + "/Business.exe";
        if (!CSystem::createDirectoriesUtf8(base) ||
            !CopyFileW(CStringManager::Utf8ToUnicode(m_binaries + "/NetWorkTest1.0.exe").c_str(),
                CStringManager::Utf8ToUnicode(businessPath).c_str(), TRUE))
        {
            throw std::runtime_error("Cannot prepare the fresh business deployment");
        }
        if (layout != 2)
        {
            if (!CSystem::createDirectoriesUtf8(targetDirectory) ||
                !CopyFileW(CStringManager::Utf8ToUnicode(executable).c_str(), CStringManager::Utf8ToUnicode(target).c_str(), TRUE) ||
                !CopyFileW(CStringManager::Utf8ToUnicode(guardExecutable).c_str(), CStringManager::Utf8ToUnicode(targetGuard).c_str(), TRUE))
            {
                throw std::runtime_error("Cannot prepare candidate gateway deployment");
            }
            if (layout == 0 && !ProcessSuiteWrite(base + "/NetClientManager1.1.exe", "invalid stale private image"))
            {
                throw std::runtime_error("Cannot prepare sibling-precedence negative fixture");
            }
        }
        NetTestProcess& business = launchPath("background-layout-" + std::to_string(layout), businessPath, { "--client-launch-probe" });
        exited(business, 7000);
        const std::string line = waitLine(business, "CLIENT_LAUNCH ", 1);
        if (layout == 2)
        {
            if (line != "CLIENT_LAUNCH result=" + std::to_string(NI_NOT_FOUND) + " pid=0")
            {
                throw std::runtime_error("Missing deployment did not report NOT_FOUND without a PID");
            }
        }
        else
        {
            const std::string prefix = "CLIENT_LAUNCH result=0 pid=";
            if (line.compare(0, prefix.size(), prefix) != 0)
            {
                throw std::runtime_error("Valid Unicode deployment did not start");
            }
            const uint32_t pid = number(line, "pid=");
            const std::chrono::steady_clock::time_point ready = std::chrono::steady_clock::now() + std::chrono::seconds(6);
            while (!CSystem::isProcessLeaseReady("NetClientManager", pid) && std::chrono::steady_clock::now() < ready)
            {
                CSystem::Sleep(10);
            }
            if (!CSystem::isProcessLeaseReady("NetClientManager", pid))
            {
                throw std::runtime_error("Selected deployment gateway did not become ready");
            }
            std::vector<uint32_t> guards;
            if (!business.ownedProcesses(targetGuard, guards) || guards.size() != 1)
            {
                throw std::runtime_error("Deployment did not start exactly one sibling guard");
            }
            windows.window(business, targetGuard, guards[0]);
            NetGatewayWindowsProbe::message(windows.window(business, target, pid), pid, WM_CLOSE);
            const std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() + std::chrono::seconds(7);
            std::vector<uint32_t> remaining;
            do
            {
                if (!business.ownedProcesses(target, remaining) || !business.ownedProcesses(targetGuard, guards))
                {
                    throw std::runtime_error("Cannot observe deployment shutdown");
                }
                if (remaining.empty() && guards.empty()) { break; }
                CSystem::Sleep(10);
            } while (std::chrono::steady_clock::now() < deadline);
            if (!remaining.empty() || !guards.empty()) { throw std::runtime_error("Deployment stayed alive after close"); }
        }
    }
    const std::string incomplete = m_output + "/missing guard";
    if (!CSystem::createDirectoriesUtf8(incomplete) ||
        !CopyFileW(CStringManager::Utf8ToUnicode(executable).c_str(),
            CStringManager::Utf8ToUnicode(incomplete + "/NetClientManager1.1.exe").c_str(), TRUE))
    {
        throw std::runtime_error("Cannot prepare missing-guard fixture");
    }
    NetTestProcess& missing = launchPath("background-missing-guard", incomplete + "/NetClientManager1.1.exe",
        { "--address=127.0.0.1", "--port=" + std::to_string(port) });
    int32_t code = 0;
    std::string missingLog;
    if (missing.wait(5000, code) != 1 || code != 1 || !missing.output(missingLog) ||
        missingLog.find("missing NetClientManagerGuard1.1.exe") == std::string::npos ||
        missingLog.find("CLIENT_GATEWAY_READY") != std::string::npos)
    {
        throw std::runtime_error("Missing guard did not prevent readiness");
    }
#else
    (void)port;
    throw std::runtime_error("Windows deployment fixture unavailable");
#endif
}

void NetGatewayProcessSuite::startup()
{
#ifdef _WIN32
    NetTestStartupEntry original(m_output + "/startup-original.bin");
    const std::string executable = m_binaries + "/NetClientManager1.1.exe";
    NetTestProcess& install = launch("background-install", "NetClientManager1.1.exe", { "--install-autostart" }, false);
    exited(install, 5000);
    const std::wstring input = CStringManager::Utf8ToUnicode(executable);
    const DWORD required = GetFullPathNameW(input.c_str(), 0, nullptr, nullptr);
    if (required == 0 || required > 32768) { throw std::runtime_error("Cannot resolve startup image"); }
    std::vector<wchar_t> full(required + 1);
    const DWORD count = GetFullPathNameW(input.c_str(), static_cast<DWORD>(full.size()), full.data(), nullptr);
    if (count == 0 || count >= full.size() || !original.matches(L"\"" + std::wstring(full.data()) + L"\" --background"))
    {
        throw std::runtime_error("Installed startup command does not exactly quote the gateway path");
    }
    for (uint32_t index = 0; index < 2; ++index)
    {
        NetTestProcess& remove = launch("background-uninstall-" + std::to_string(index), "NetClientManager1.1.exe",
            { "--uninstall-autostart" }, false);
        exited(remove, 5000);
        if (!original.absent()) { throw std::runtime_error("Uninstall left the startup value"); }
    }
    if (!original.restore()) { throw std::runtime_error("Cannot restore original startup state"); }
#else
    throw std::runtime_error("Windows startup fixture unavailable");
#endif
}

void NetGatewayProcessSuite::luma(const std::string& serverExecutable, const std::string& testExecutable)
{
    NetTestProcess& selection = launchPath("luma-selection", testExecutable, {});
    exited(selection, 15000);
    if (waitLine(selection, "Gateway selection ", 1) != "Gateway selection 9/9")
    {
        throw std::runtime_error("Luma gateway selection boundary checks incomplete");
    }
    NetTestProcess& gateway = launch("luma-gateway", "NetServerManager1.1.exe", { "--protocol=3", "--listen=127.0.0.1",
        "--advertise=127.0.0.1", "--port=0", "--seeds=127.0.0.1:1", "--identity-directory=" + m_output + "/luma-identity" }, true);
    const std::string ready = waitLine(gateway, "GATEWAY_READY ", 12000);
    const uint32_t port = number(ready, "port=");
    if (number(ready, "pid=") != gateway.pid() || port > 65535) { throw std::runtime_error("Luma fixture gateway identity mismatch"); }
    m_residents.push_back("luma-service");
    NetTestProcess& service = launchPath("luma-service", serverExecutable, { "--gateway-pid=" + std::to_string(gateway.pid()),
        "--stop-file=" + m_output + "/luma-service.stop", "--run-for-ms=60000" });
    const std::string registered = waitLine(service, "LUMA_SERVICE_READY ", 12000);
    if (number(registered, "pid=") != service.pid() || number(registered, "product=") != 101 ||
        number(registered, "endpoint=") == 0) { throw std::runtime_error("Luma business registration identity mismatch"); }
    NetTestProcess& desktop = launch("luma-desktop", "NetClientManager1.1.exe",
        { "--protocol=3", "--address=127.0.0.1", "--port=" + std::to_string(port),
            "--seeds=127.0.0.1:" + std::to_string(port), "--cache-file=" + m_output + "/luma.cache" }, true);
    if (number(waitLine(desktop, "CLIENT_GATEWAY_READY ", 12000), "pid=") != desktop.pid())
    {
        throw std::runtime_error("Luma desktop gateway identity mismatch");
    }
    NetTestProcess& probe = launchPath("luma-rpc", testExecutable, { "--gateway-pid=" + std::to_string(desktop.pid()) });
    exited(probe, 15000);
    if (waitLine(probe, "Gateway selection ", 1) != "Gateway selection 9/9" ||
        waitLine(probe, "PASS desktop IPC ", 1) != "PASS desktop IPC -> gateway -> LumaPlayerServer: failures/cancel=3/3")
    {
        throw std::runtime_error("Luma RPC/cancellation actual terminal checks incomplete");
    }
    int32_t code = 0;
    if (gateway.poll(code) != 0 || service.poll(code) != 0 || desktop.poll(code) != 0)
    {
        throw std::runtime_error("Luma fixture resident exited during business verification");
    }
    stop("luma-service");
    exited(service, 5000);
    if (waitLine(service, "LUMA_SERVICE_STOPPED", 1) != "LUMA_SERVICE_STOPPED")
    {
        throw std::runtime_error("Luma service did not confirm orderly shutdown");
    }
    stop("luma-desktop");
    exited(desktop, 5000);
    descendantsStopped(desktop, "NetClientManagerGuard1.1.exe", 5000);
    stop("luma-gateway");
    exited(gateway, 5000);
}

int NetGatewayProcessSuite::run(int argc, char** argv)
{
    std::vector<std::string> arguments;
    if (!CSystem::commandArguments(argc, argv, arguments) || arguments.empty())
    {
        return 2;
    }
    if (arguments.size() == 1 && arguments[0] == "--process-fixture=parser")
    {
        try
        {
            if (number("READY port=65535 pid=17", "port=") != 65535 ||
                number("READY pid=4294967295", "pid=") != UINT32_MAX)
            {
                return 1;
            }
            const char* invalid[] = { "READY", "pid=", "pid=0", "pid=-1", "pid=+1", "pid=12x",
                "pid=4294967296", "pid=999999999999999", "pid=1 pid=2", "prefixpid=1", "pid= 1" };
            for (size_t index = 0; index < sizeof(invalid) / sizeof(invalid[0]); ++index)
            {
                bool rejected = false;
                try
                {
                    number(invalid[index], "pid=");
                }
                catch (const std::runtime_error&)
                {
                    rejected = true;
                }
                if (!rejected)
                {
                    return 1;
                }
            }
            std::puts("PROCESS_FIXTURE_PARSER checks=13 PASS");
            return 0;
        }
        catch (...)
        {
            return 1;
        }
    }
    const bool isSingleton = arguments[0] == "--process-fixture=singleton";
    const bool isBackground = arguments[0] == "--process-fixture=background";
    const bool isLuma = arguments[0] == "--process-fixture=luma";
    if (arguments[0] != "--process-fixture=replacement" && !isSingleton && !isBackground && !isLuma)
    {
        return 2;
    }
    std::string binaries;
    std::string output;
    std::string lumaServer;
    std::string lumaTest;
    for (size_t index = 1; index < arguments.size(); ++index)
    {
        const std::string& argument = arguments[index];
        if (argument.compare(0, 6, "--bin=") == 0 && binaries.empty() && argument.size() > 6)
        {
            binaries = argument.substr(6);
        }
        else if (argument.compare(0, 9, "--output=") == 0 && output.empty() && argument.size() > 9)
        {
            output = argument.substr(9);
        }
        else if (isLuma && argument.compare(0, 14, "--luma-server=") == 0 && lumaServer.empty() && argument.size() > 14)
        {
            lumaServer = CSystem::canonicalPathUtf8(argument.substr(14), true);
            if (lumaServer.empty()) { return 2; }
        }
        else if (isLuma && argument.compare(0, 19, "--luma-server-test=") == 0 && lumaTest.empty() && argument.size() > 19)
        {
            lumaTest = CSystem::canonicalPathUtf8(argument.substr(19), true);
            if (lumaTest.empty()) { return 2; }
        }
        else
        {
            return 2;
        }
    }
#ifdef _WIN32
    if (binaries.empty() || output.empty() || (isLuma && (lumaServer.empty() || lumaTest.empty())) || !leasesFree())
    {
        std::fputs("Fixture requires candidates, a fresh directory and unoccupied gateway leases\n", stderr);
        return 2;
    }
    output = CSystem::canonicalPathUtf8(output, false);
    binaries = CSystem::canonicalPathUtf8(binaries, true);
    if (output.empty() || binaries.empty() || !CreateDirectoryW(CStringManager::Utf8ToUnicode(output).c_str(), nullptr))
    {
        return 2;
    }
    std::string state = "FAIL";
    try
    {
        NetGatewayProcessSuite fixture(binaries, output);
        if (isSingleton) { fixture.singleton(); }
        else if (isBackground) { fixture.background(); }
        else if (isLuma) { fixture.luma(lumaServer, lumaTest); }
        else { fixture.replacement(); }
        state = "PASS";
    }
    catch (const std::exception& error)
    {
        std::fprintf(stderr, "Gateway process fixture failed: %s\n", error.what());
    }
    catch (...)
    {
        std::fputs("Gateway process fixture failed with an unexpected exception\n", stderr);
    }
    const std::string suite = isSingleton ? "singleton" : isBackground ? "background" : isLuma ? "luma" : "replacement";
    if (!ProcessSuiteWrite(output + "/result.csv", "suite,state\n" + suite + "," + state + "\n"))
    {
        return 3;
    }
    std::printf("Gateway %s %s\n", suite.c_str(), state.c_str());
    return state == "PASS" ? 0 : 1;
#else
    (void)binaries;
    (void)output;
    std::fputs("Production Windows lifecycle fixture: NOT_RUN on this platform\n", stderr);
    return 2;
#endif
}