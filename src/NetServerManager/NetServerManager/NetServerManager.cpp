#include "NetServerManager.h"
#include "NetGatewayTask.h"
#include "NetGatewayAuthority.h"
#include "NetWork/NetWorkAPI.h"
#include "LibuvTcp/LibuvTcpAPI.h"
#include "CSystem/CSystemAPI.h"
// 原生文件后端由NetWork正式宿主入口配置。
#include "NetGatewayIdentity.h"
#include "NetGatewayServiceProbe.h"
#include "NetGatewayManagedProbe.h"
#include "NetInterface/NetInterfaceAPI.h"
#include <cstdio>
#include <csignal>
#include <map>
#include <sstream>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#pragma comment(lib, "shell32.lib")
#endif

// 标准信号处理器与主循环共享的退出位，只做信号安全的赋值
static volatile sig_atomic_t g_gatewaySignal = 0;
#ifdef _WIN32
// Windows控制台回调线程通知主循环停止，由主循环回收服务资源
static std::atomic<bool> g_gatewayConsoleStop(false);

/** 系统回调仅置停止标志
@param [in] event 控制台事件
@return 是否接纳停止
*/
static BOOL WINAPI GatewayConsoleSignal(DWORD event)
{
    if (event == CTRL_C_EVENT || event == CTRL_BREAK_EVENT || event == CTRL_CLOSE_EVENT || event == CTRL_SHUTDOWN_EVENT ||
        event == CTRL_LOGOFF_EVENT)
    {
        g_gatewayConsoleStop.store(true);
        return TRUE;
    }
    return FALSE;
}

#endif

/** 标准信号处理只写标准信号标志
@param [in] signal 信号
*/
static void GatewaySignal(int signal)
{
    (void)signal;
    g_gatewaySignal = 1;
}

/** 按UTF8路径只读打开，不依赖Windows ANSI代码页
@param [in] path 本地路径
@return 文件或空，调用者fclose
*/
static FILE* OpenGatewayFile(const std::string& path)
{
#ifdef _WIN32
    const int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path.c_str(), -1, nullptr, 0);
    if (size <= 0)
    {
        return nullptr;
    }
    std::vector<wchar_t> value(size);
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path.c_str(), -1, value.data(), size) == 0)
    {
        return nullptr;
    }
    return _wfopen(value.data(), L"rb");
#else
    return std::fopen(path.c_str(), "rb");
#endif
}

/** 有界读取PEM，不输出文件内容
@param [in] path UTF8路径
@param [in] maximum 最大字节
@param [out] output 成功文件字节，失败不改变
@return 是否成功
*/
static bool ReadGatewayFile(const std::string& path, uint32_t maximum, std::string& output)
{
    std::vector<char> bytes(maximum + 1);
    FILE* file = OpenGatewayFile(path);
    if (file == nullptr)
    {
        return false;
    }
    const size_t size = std::fread(bytes.data(), 1, bytes.size(), file);
    const bool failed = std::ferror(file) != 0;
    std::fclose(file);
    if (failed || size == 0 || size > maximum)
    {
        return false;
    }
    output.assign(bytes.data(), size);
    return true;
}

/** 严格十进制解析，不允许符号、溢出及垃圾
@param [in] text 参数
@param [in] maximum 上限
@param [out] output 成功无符号值
@return 是否合法
*/
static bool GatewayNumber(const std::string& text, uint32_t maximum, uint32_t& output)
{
    if (text.empty() || text.size() > 10 || text.find_first_not_of("0123456789") != std::string::npos)
    {
        return false;
    }
    uint64_t value = 0;
    for (char character : text)
    {
        value = value * 10 + static_cast<uint32_t>(character - '0');
    }
    if (value > maximum)
    {
        return false;
    }
    output = static_cast<uint32_t>(value);
    return true;
}

/** 只读产品=64位十六进制秘密配置，不把秘密放进命令行或日志
@param [in] path UTF8配置路径
@param [out] output 成功1至32项
@return 是否合法
*/
static bool ReadGatewayServices(const std::string& path, std::vector<NetGatewayServiceCredential>& output)
{
    std::string bytes;
    if (!ReadGatewayFile(path, 8192, bytes))
    {
        return false;
    }
    std::istringstream input(bytes);
    std::vector<NetGatewayServiceCredential> result;
    std::map<uint32_t, bool> seen;
    std::string line;
    while (std::getline(input, line))
    {
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }
        if (line.empty() || line[0] == '#')
        {
            continue;
        }
        const size_t separator = line.find('=');
        if (separator == std::string::npos || line.size() - separator - 1 != 64 || result.size() >= 32)
        {
            return false;
        }
        NetGatewayServiceCredential credential = {};
        if (!GatewayNumber(line.substr(0, separator), UINT32_MAX, credential.m_product))
        {
            return false;
        }
        if (!seen.insert(std::make_pair(credential.m_product, true)).second)
        {
            return false;
        }
        for (uint32_t index = 0; index < 64; ++index)
        {
            const char byte = line[separator + 1 + index];
            const uint32_t nibble = byte >= '0' && byte <= '9'   ? byte - '0'
                                    : byte >= 'a' && byte <= 'f' ? byte - 'a' + 10
                                    : byte >= 'A' && byte <= 'F' ? byte - 'A' + 10
                                                                 : 16;
            if (nibble == 16)
            {
                return false;
            }
            credential.m_secret[index / 2] = static_cast<uint8_t>(credential.m_secret[index / 2] << 4 | nibble);
        }
        if (NetInterfaceService::hello(credential.m_secret, 32, nullptr, 0) != NI_BUFFER_TOO_SMALL)
        {
            return false;
        }
        result.push_back(credential);
    }
    if (result.empty())
    {
        return false;
    }
    output.swap(result);
    return true;
}

/** 本程序入口将LibuvTcp接入通用宿主表；NetWork不选择具体实现
@return NI_OK或错误码
*/
static int32_t ConfigureGatewayHost()
{
    NetHostFunctions functions = {};
    functions.structSize = sizeof(functions);
    functions.version = 1;
    functions.create = LibuvTcp::hostCreate;
    functions.listen = LibuvTcp::hostListen;
    functions.connect = LibuvTcp::hostConnect;
    functions.poll = LibuvTcp::hostPoll;
    functions.write = LibuvTcp::hostWrite;
    functions.pauseRead = LibuvTcp::hostPauseRead;
    functions.close = LibuvTcp::hostClose;
    functions.shutdown = LibuvTcp::hostShutdown;
    functions.destroy = LibuvTcp::hostDestroy;
    functions.localPort = LibuvTcp::hostLocalPort;
    functions.peerCount = LibuvTcp::hostPeerCount;
    functions.entropy = CSystem::secureRandomBytesCallback;
    return NetWork::configureHost(functions);
}

NetServerManager::NetServerManager() :
m_stop(false),
m_hostConfigured(false),
m_initialized(false),
m_running(false),
m_completed(false),
m_cleanupFailed(false),
m_thread(0),
m_previousInterrupt(nullptr),
m_previousTerminate(nullptr),
m_interruptInstalled(false),
m_terminateInstalled(false),
m_consoleInstalled(false)
{

}

NetServerManager::~NetServerManager()
{
    uninit();
}

NetServerManager& NetServerManager::instance()
{
    // 先构造任务管理器，保证本管理对象析构兜底时任务管理器仍然存在；此处不创建线程。
    CTaskThreadManager::Instance();
    static NetServerManager s_manager;
    return s_manager;
}

int NetServerManager::init(const std::vector<std::string>& arguments)
{
    if (m_initialized || m_running || m_thread != 0 || m_task || !m_lease.empty() ||
        m_interruptInstalled || m_terminateInstalled || m_consoleInstalled)
    {
        return 1;
    }
    m_stop.store(false);
    m_completed = false;
    m_cleanupFailed = false;
    g_gatewaySignal = 0;
#ifdef _WIN32
    g_gatewayConsoleStop.store(false);
#endif
    int result = 1;
    try
    {
        result = prepare(arguments);
    }
    catch (...)
    {
        std::fputs("NetServerManager startup failed\n", stderr);
    }
    if (result != 0)
    {
        return uninit() ? result : 1;
    }
    m_initialized = true;
    return 0;
}

int NetServerManager::prepare(const std::vector<std::string>& arguments)
{
    if (!m_hostConfigured)
    {
        // NetWork同时登记默认文件后端；本对象后续init继续使用同一份进程宿主表。
        if (ConfigureGatewayHost() != NI_OK)
        {
            return 1;
        }
        m_hostConfigured = true;
    }
    // 在取得进程租约和准备身份文件前拒绝旧入口，避免误用时产生副作用。
    for (const std::string& argument : arguments)
    {
        if (argument == "--legacy" || argument.compare(0, 9, "--legacy=") == 0)
        {
            std::fputs("Legacy gateway mode has been removed; use --protocol=3\n", stderr);
            return 2;
        }
    }
    if (arguments.size() == 1 && arguments[0] == "--self-test=identity")
    {
        return NetGatewayAuthority::selfTest() == NI_OK ? 0 : 1;
    }
    if (arguments.size() == 1 && arguments[0] == "--self-test=link")
    {
        return NetGatewayAuthority::linkSelfTest() == NI_OK ? 0 : 1;
    }
    if (arguments.size() == 3 && (arguments[0].compare(0, 20, "--self-test-managed=") == 0 ||
        arguments[0].compare(0, 29, "--self-test-managed-internal=") == 0 ||
        arguments[0].compare(0, 21, "--self-test-capacity=") == 0) &&
        arguments[1].compare(0, 12, "--stop-file=") == 0 && arguments[2].compare(0, 13, "--run-for-ms=") == 0)
    {
        uint32_t pid = 0;
        uint32_t duration = 0;
        if (!GatewayNumber(arguments[0].substr(arguments[0].find('=') + 1), INT32_MAX, pid) ||
            !GatewayNumber(arguments[2].substr(13), 60000, duration)) { return 2; }
        return NetGatewayManagedProbe::run(static_cast<int32_t>(pid), arguments[1].substr(12), duration,
            arguments[0].find("-internal=") != std::string::npos, arguments[0].find("-capacity=") != std::string::npos);
    }
    if (arguments.size() == 3 && (arguments[0].compare(0, 20, "--self-test-service=") == 0 ||
        arguments[0].compare(0, 21, "--self-test-internal=") == 0 || arguments[0].compare(0, 18, "--self-test-query=") == 0) &&
        arguments[1].compare(0, 12, "--stop-file=") == 0 && arguments[2].compare(0, 13, "--run-for-ms=") == 0)
    {
        uint32_t pid = 0;
        uint32_t duration = 0;
        if (!GatewayNumber(arguments[0].substr(arguments[0].find('=') + 1), INT32_MAX, pid) ||
            !GatewayNumber(arguments[2].substr(13), NetGatewayServiceProbe::MaximumDurationMs, duration)) { return 2; }
        const bool internal = arguments[0].compare(0, 21, "--self-test-internal=") == 0;
        const bool query = arguments[0].compare(0, 18, "--self-test-query=") == 0;
        return NetGatewayServiceProbe::run(static_cast<int32_t>(pid), arguments[1].substr(12), duration, internal ? 0 : 1, query);
    }
    if (arguments.size() == 1 && arguments[0] == "--help")
    {
        std::puts("NetServerManager [--identity-directory=gateway] [--listen=127.0.0.1] [--port=5203]");
        std::puts("Existing compatibility mode: --certificate=<PEM> --private-key=<PEM> (paired options).");
        std::puts("NTW3: --protocol=3 [--advertise=<reachable numeric address>], requires automatic node identity.");
        std::puts(
            "Optional: --services=<credential file> --max-peers=256 --run-for-ms=0 --stop-file=<owned path>.");
        return 0;
    }
    std::string isolated;
#ifdef NET_GATEWAY_TEST_INSTANCE
    // 只用于独立命名的NetWorkTest宿主；正式两个网关从不编译此宏。
    isolated = std::to_string(CSystem::currentProcessPid());
#endif
    const int32_t acquired = CSystem::acquireProcessLease("NetServerManager", arguments.empty(), 10000, m_lease, isolated);
    if (acquired != 1)
    {
        std::fputs(acquired == 0 ? "NetServerManager_REUSED: existing instance retained; arguments not applied\n" :
            "NetServerManager lease unavailable or previous instance did not stop in 10 seconds\n", stderr);
        return acquired == 0 ? 0 : 1;
    }
    std::map<std::string, std::string> options;
    for (const std::string& argument : arguments)
    {
        const size_t separator = argument.find('=');
        const std::string key = argument.substr(0, separator);
        if (separator == std::string::npos || separator + 1 == argument.size() ||
            (key != "--certificate" && key != "--private-key" && key != "--listen" && key != "--port" && key != "--max-peers" &&
             key != "--run-for-ms" && key != "--stop-file" && key != "--services" && key != "--identity-directory" &&
             key != "--protocol" && key != "--advertise" && key != "--seeds") ||
            options.count(key))
        {
            std::fputs("Invalid gateway argument; use --help\n", stderr);
            return 2;
        }
        options[key] = argument.substr(separator + 1);
    }
    if (options.count("--certificate") != options.count("--private-key") ||
        (options.count("--certificate") && options.count("--identity-directory")))
    {
        std::fputs("Invalid identity configuration; use --help\n", stderr);
        return 2;
    }
    NetGatewayConfiguration configuration;
    configuration.m_port = 5203;
    configuration.m_peers = 256;
    configuration.m_runForMs = 0;
    configuration.m_address = options.count("--listen") ? options["--listen"] : "0.0.0.0";
    if ((options.count("--port") && !GatewayNumber(options["--port"], 65535, configuration.m_port)) ||
        (options.count("--max-peers") && !GatewayNumber(options["--max-peers"], 256, configuration.m_peers)) ||
        (options.count("--run-for-ms") && !GatewayNumber(options["--run-for-ms"], 86400000, configuration.m_runForMs)))
    {
        std::fputs("Invalid gateway configuration\n", stderr);
        return 2;
    }
    if ((options.count("--protocol") && (!GatewayNumber(options["--protocol"], 3, configuration.m_protocol) ||
        configuration.m_protocol < 2)) || (configuration.m_protocol == 3 && (options.count("--certificate") || options.count("--services"))))
    {
        std::fputs("Invalid protocol configuration; NTW3 uses automatic identity and registration\n", stderr);
        return 2;
    }
    // 当前正式入口预置一台已知服务器；新增主机必须公布其实际可达地址。
    configuration.m_advertise = options.count("--advertise") ? options["--advertise"] :
        configuration.m_address == "0.0.0.0" ? "175.24.198.162" : configuration.m_address;
    if (options.count("--seeds"))
    {
        if (configuration.m_protocol != 3) { std::fputs("Seeds require protocol 3\n", stderr); return 2; }
        std::istringstream seeds(options["--seeds"]);
        std::string seed;
        while (std::getline(seeds, seed, ','))
        {
            const size_t colon = seed.rfind(':');
            uint32_t port = 0;
            std::string address = colon == std::string::npos ? "" : seed.substr(0, colon);
            if (address.size() >= 2 && address.front() == '[' && address.back() == ']') { address = address.substr(1, address.size() - 2); }
            if (configuration.m_seeds.size() >= 64 || !NetInterfaceDirectory::address(address) ||
                !GatewayNumber(seed.substr(colon + 1), 65535, port) || port == 0)
            {
                std::fputs("Invalid numeric seed list\n", stderr);
                return 2;
            }
            configuration.m_seeds.push_back(std::make_pair(address, port));
        }
        if (configuration.m_seeds.empty() || options["--seeds"].back() == ',') { return 2; }
    }
    if (options.count("--certificate"))
    {
        if (!ReadGatewayFile(options["--certificate"], 131071, configuration.m_certificate) ||
            !ReadGatewayFile(options["--private-key"], 32767, configuration.m_privateKey))
        {
            std::fputs("Invalid legacy TLS configuration\n", stderr);
            return 2;
        }
    }
    else
    {
        NetGatewayIdentity identity;
        const std::string directory = options.count("--identity-directory") ? options["--identity-directory"] : "gateway";
        configuration.m_identityDirectory = directory;
        if (identity.loadOrCreate(directory) != NI_OK)
        {
            std::fputs("Cannot validate or create gateway identity.dat; existing damaged identity was not overwritten\n", stderr);
            return 2;
        }
        configuration.m_certificate = identity.m_certificate;
        configuration.m_privateKey = identity.m_privateKey;
        configuration.m_nodeName = identity.m_name;
    }
    if (options.count("--services"))
    {
        if (!ReadGatewayServices(options["--services"], configuration.m_services))
        {
            std::fputs("Invalid gateway service configuration\n", stderr);
            return 2;
        }
    }
    m_stopFile = options.count("--stop-file") ? options["--stop-file"] : "";
    if (!m_stopFile.empty())
    {
        FILE* existing = OpenGatewayFile(m_stopFile);
        if (existing != nullptr)
        {
            std::fclose(existing);
            std::fputs("Stop file already exists; use a fresh owned path\n", stderr);
            return 2;
        }
    }
    if (!installSignals())
    {
        return 1;
    }
    m_task.reset(new NetGatewayTask(configuration));
    m_thread = CTaskThreadManager::Instance().Init();
    if (m_thread == 0)
    {
        return 1;
    }
    std::shared_ptr<CTaskThread> executor = CTaskThreadManager::Instance().GetThreadInterface(m_thread);
    if (!executor)
    {
        std::fputs("Cannot create gateway executor\n", stderr);
        return 1;
    }
    executor->PostTask(m_task);
    executor.reset();
    return 0;
}

bool NetServerManager::installSignals()
{
    m_previousInterrupt = std::signal(SIGINT, GatewaySignal);
    if (m_previousInterrupt == SIG_ERR)
    {
        return false;
    }
    m_interruptInstalled = true;
    m_previousTerminate = std::signal(SIGTERM, GatewaySignal);
    if (m_previousTerminate == SIG_ERR)
    {
        return false;
    }
    m_terminateInstalled = true;
#ifdef _WIN32
    if (!SetConsoleCtrlHandler(GatewayConsoleSignal, TRUE))
    {
        return false;
    }
    m_consoleInstalled = true;
#endif
    return true;
}

void NetServerManager::stop()
{
    m_stop.store(true);
}

bool NetServerManager::stopTask()
{
    stop();
    try
    {
        if (!m_lease.empty())
        {
            CSystem::setProcessLeaseReady(m_lease, false);
        }
        if (m_task)
        {
            m_task->StopTask();
        }
        if (m_thread != 0)
        {
            CTaskThreadManager::Instance().WaitForEnd(m_thread);
            m_thread = 0;
        }
        return true;
    }
    catch (...)
    {
        m_cleanupFailed = true;
        std::fputs("NetServerManager cannot finish task cleanup\n", stderr);
        return false;
    }
}

int NetServerManager::run()
{
    if (!m_initialized || m_running || m_completed)
    {
        return 1;
    }
    if (!m_task)
    {
        m_completed = true;
        return 0;
    }
    m_running = true;
    int result = 1;
    try
    {
        bool announced = false;
        while (!m_task->done())
        {
            if (!announced && m_task->port() != 0 && !m_stop.load() && !CSystem::processLeaseStopRequested(m_lease))
            {
                if (!CSystem::setProcessLeaseReady(m_lease, true))
                {
                    stop();
                    m_task->StopTask();
                    break;
                }
                std::printf("GATEWAY_READY port=%u pid=%u\n", m_task->port(), static_cast<uint32_t>(CSystem::currentProcessPid()));
                std::fflush(stdout);
                announced = true;
            }
            bool stopRequested = m_stop.load() || g_gatewaySignal != 0 || CSystem::processLeaseStopRequested(m_lease);
#ifdef _WIN32
            stopRequested = stopRequested || g_gatewayConsoleStop.load();
#endif
            if (!m_stopFile.empty())
            {
                FILE* requested = OpenGatewayFile(m_stopFile);
                if (requested != nullptr)
                {
                    std::fclose(requested);
                    stopRequested = true;
                }
            }
            if (stopRequested)
            {
                CSystem::setProcessLeaseReady(m_lease, false);
                stop();
                m_task->StopTask();
            }
            CSystem::Sleep(10);
        }
        if (stopTask() && !m_cleanupFailed && m_task->error() == 0)
        {
            std::puts("GATEWAY_STOPPED");
            result = 0;
        }
        else
        {
            std::fprintf(stderr, "NetServerManager stopped with error %d\n", m_task->error());
        }
    }
    catch (...)
    {
        std::fputs("NetServerManager run failed\n", stderr);
        stopTask();
    }
    m_running = false;
    m_completed = true;
    return result;
}

bool NetServerManager::uninit()
{
    if (m_running)
    {
        stop();
        return false;
    }
    if (!stopTask())
    {
        return false;
    }
    m_task.reset();
    m_stopFile.clear();
    bool restored = true;
#ifdef _WIN32
    if (m_consoleInstalled)
    {
        if (SetConsoleCtrlHandler(GatewayConsoleSignal, FALSE))
        {
            m_consoleInstalled = false;
        }
        else
        {
            restored = false;
        }
    }
#endif
    if (m_interruptInstalled)
    {
        if (std::signal(SIGINT, m_previousInterrupt) != SIG_ERR)
        {
            m_interruptInstalled = false;
        }
        else
        {
            restored = false;
        }
    }
    if (m_terminateInstalled)
    {
        if (std::signal(SIGTERM, m_previousTerminate) != SIG_ERR)
        {
            m_terminateInstalled = false;
        }
        else
        {
            restored = false;
        }
    }
    CSystem::releaseProcessLease(m_lease);
    m_initialized = false;
    m_completed = false;
    return restored;
}

/** 进程入口，只读取平台参数并顺序调用管理器的生命周期
@param [in] argc 原平台参数数量
@param [in] argv 原平台参数，Windows经CSystem读取Unicode命令行
@return 正常退出或复用0，启动运行失败1，参数错误2
*/
int main(int argc, char** argv)
{
    try
    {
        std::vector<std::string> arguments;
        if (!CSystem::commandArguments(argc, argv, arguments))
        {
            return 2;
        }
        NetServerManager& manager = NetServerManager::instance();
        int result = manager.init(arguments);
        if (result == 0)
        {
            result = manager.run();
        }
        return manager.uninit() ? result : 1;
    }
    catch (...)
    {
        std::fputs("NetServerManager entry failed\n", stderr);
        return 1;
    }
}

#if 0 // MANAGER_LIFECYCLE_UNIT_BEGIN
// 测试时在隔离副本停用上方生产main、启用此main，并定义NET_GATEWAY_TEST_INSTANCE；参数为独占临时目录。
// 测试结束保持此块关闭，不运行生产单例或实际公网端点。
#if defined(_WIN32) && defined(_DEBUG)
#include <crtdbg.h>

// 只拒绝当前测试主线程的一次分配，工作线程不参与故障注入。
static DWORD g_managerAllocationThread = 0;
// 剩余主线程分配次数，负数关闭注入。
static int g_managerAllocationBudget = -1;
// 当前注入点是否实际拒绝过分配。
static bool g_managerAllocationRejected = false;
// 测试开始前已有的CRT分配钩子，未拒绝时继续调用。
static _CRT_ALLOC_HOOK g_managerPreviousAllocationHook = nullptr;

/** 仅在选定主线程分配点注入一次失败，释放动作及其他线程保持原行为
@param [in] operation CRT分配操作
@param [in] pointer 原块
@param [in] size 请求字节
@param [in] block CRT块类型
@param [in] request 分配编号
@param [in] filename CRT源文件
@param [in] line CRT源行
@return 是否允许本次CRT操作
*/
static int __cdecl ManagerAllocationHook(int operation, void* pointer, size_t size, int block, long request,
    const unsigned char* filename, int line)
{
    if (operation == _HOOK_ALLOC && GetCurrentThreadId() == g_managerAllocationThread && g_managerAllocationBudget >= 0)
    {
        if (g_managerAllocationBudget-- == 0)
        {
            g_managerAllocationRejected = true;
            return FALSE;
        }
    }
    return g_managerPreviousAllocationHook == nullptr ? TRUE :
        g_managerPreviousAllocationHook(operation, pointer, size, block, request, filename, line);
}
#endif

/** 管理类生命周期单测，覆盖重复调用、失败恢复、停止、信号恢复及分配失败回收
@param [in] argc 原平台参数数量
@param [in] argv 原平台参数，唯一业务参数为独占临时目录
@return 0通过，1检查失败，2参数错误
*/
int main(int argc, char** argv)
{
#ifdef _WIN32
    if (!CSystem::bindInheritedOutput())
    {
        return 1;
    }
#endif
    NetServerManager& manager = NetServerManager::instance();
    try
    {
        std::vector<std::string> paths;
        if (!CSystem::commandArguments(argc, argv, paths) || paths.size() != 1)
        {
            return 2;
        }
        uint32_t checks = 0;
        // condition为独立预期，message只含用例名；失败打印后交给main统一清理。
        const auto check = [&checks](bool condition, const char* message) -> bool
        {
            ++checks;
            if (!condition)
            {
                std::fprintf(stderr, "MANAGER_UNIT_FAILED %s\n", message);
            }
            return condition;
        };
        if (!check(&manager == &NetServerManager::instance(), "singleton") ||
            !check(manager.run() == 1, "run-before-init") ||
            !check(manager.uninit() && manager.uninit(), "empty-uninit"))
        {
            return 1;
        }
        std::vector<std::string> help(1, "--help");
        if (!check(manager.init(help) == 0, "help-init") ||
            !check(manager.init(help) == 1, "duplicate-init") ||
            !check(manager.run() == 0, "help-run") ||
            !check(manager.run() == 1, "duplicate-run") ||
            !check(manager.uninit() && manager.uninit(), "help-uninit"))
        {
            return 1;
        }
        std::vector<std::string> invalid(1, "--port=70000");
        if (!check(manager.init(invalid) == 2, "invalid-config") ||
            !check(manager.run() == 1, "failed-init-not-runnable") ||
            !check(manager.uninit(), "failed-init-uninit"))
        {
            return 1;
        }
        std::vector<std::string> arguments;
        arguments.push_back("--protocol=3");
        arguments.push_back("--listen=127.0.0.1");
        arguments.push_back("--port=0");
        arguments.push_back("--run-for-ms=40");
        arguments.push_back("--identity-directory=" + CSystem::joinPath(paths[0], "identity"));
        // 先实际运行一轮，使第三方懒初始化与后续任务/句柄回收检查分离。
        if (!check(manager.init(arguments) == 0, "warm-init") ||
            !check(manager.run() == 0, "warm-run") || !check(manager.uninit(), "warm-uninit"))
        {
            return 1;
        }
        const int32_t threads = CTaskThreadManager::Instance().Count();
        for (uint32_t cycle = 0; cycle < 4; ++cycle)
        {
            const auto originalSignal = std::signal(SIGINT, SIG_IGN);
            if (!check(originalSignal != SIG_ERR, "test-signal-install") ||
                !check(manager.init(arguments) == 0, "cycle-init") ||
                !check(manager.init(arguments) == 1, "live-duplicate-init"))
            {
                return 1;
            }
            if (cycle == 0)
            {
                manager.stop();
                manager.stop();
            }
            else if (cycle == 1)
            {
                std::raise(SIGINT);
            }
            if (cycle != 3 && !check(manager.run() == 0, "cycle-run"))
            {
                return 1;
            }
            if (!check(manager.uninit() && manager.uninit(), "cycle-uninit") ||
                !check(CTaskThreadManager::Instance().Count() == threads, "thread-reclaimed") ||
                !check(std::signal(SIGINT, SIG_IGN) == SIG_IGN, "signal-restored"))
            {
                return 1;
            }
            std::signal(SIGINT, originalSignal);
        }
        std::vector<std::string> failed(arguments);
        failed[1] = "--listen=invalid-ip";
        if (!check(manager.init(failed) == 0, "async-failure-prepare") ||
            !check(manager.run() == 1, "async-failure-reported") ||
            !check(manager.uninit(), "async-failure-uninit") ||
            !check(CTaskThreadManager::Instance().Count() == threads, "failed-thread-reclaimed"))
        {
            return 1;
        }
        uint32_t rejected = 0;
#if defined(_WIN32) && defined(_DEBUG)
        g_managerAllocationThread = GetCurrentThreadId();
        for (int budget = 0; budget < 160; ++budget)
        {
            g_managerAllocationBudget = budget;
            g_managerAllocationRejected = false;
            g_managerPreviousAllocationHook = _CrtSetAllocHook(ManagerAllocationHook);
            const int initialized = manager.init(arguments);
            _CrtSetAllocHook(g_managerPreviousAllocationHook);
            g_managerAllocationBudget = -1;
            if (g_managerAllocationRejected)
            {
                ++rejected;
            }
            // 返回成功的注入点可能属于可选准备；同样必须可停止且收回任务。
            if (initialized == 0)
            {
                manager.stop();
                manager.run();
            }
            if (!check(manager.uninit() && manager.uninit(), "allocation-uninit") ||
                !check(CTaskThreadManager::Instance().Count() == threads, "allocation-thread-reclaimed"))
            {
                return 1;
            }
        }
        if (!check(rejected > 0, "allocation-rejection-exercised"))
        {
            return 1;
        }
#endif
        if (!check(manager.init(arguments) == 0 && manager.run() == 0 && manager.uninit(), "final-recovery"))
        {
            return 1;
        }
        std::printf("MANAGER_UNIT_PASS checks=%u rejected=%u\n", checks, rejected);
        return 0;
    }
    catch (...)
    {
#if defined(_WIN32) && defined(_DEBUG)
        _CrtSetAllocHook(g_managerPreviousAllocationHook);
        g_managerAllocationBudget = -1;
#endif
        manager.uninit();
        std::fputs("MANAGER_UNIT_FAILED unexpected-exception\n", stderr);
        return 1;
    }
}
#endif // MANAGER_LIFECYCLE_UNIT_END