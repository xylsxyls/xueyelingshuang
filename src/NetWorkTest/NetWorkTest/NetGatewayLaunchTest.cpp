#include "NetGatewayLaunchTest.h"
#include "../../NetSender/NetSender/src/NetSenderGatewayLaunch.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "NetInterface/NetInterfaceAPI.h"
#include <stdexcept>

// 仅本测试翻译单元替换系统调用；生产源码和ABI保持不变。
#define NetSenderGatewayLaunch NetGatewayLaunchSubject
#define CSystem NetGatewayLaunchSystem
#define CStringManager NetGatewayLaunchString
#include "../../NetSender/NetSender/src/NetSenderGatewayLaunch.cpp"
#undef CStringManager
#undef CSystem
#undef NetSenderGatewayLaunch

#include "../../NetSender/NetSender/src/NetSenderGatewayLaunch.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "NetInterface/NetInterfaceAPI.h"
#include <algorithm>
#include <cstdio>

// 仅STATIC_LIB隔离单元的OS查询替身，不启动/关闭进程或注册开机项
static std::vector<uint32_t> s_launchTestMains;
static std::vector<uint32_t> s_launchTestGuards;
static std::vector<std::string> s_launchTestQueries;
static bool s_launchTestMainReady = true;
static bool s_launchTestGuardReady = true;
static bool s_launchTestSibling = false;
static bool s_launchTestFlat = false;
static uint32_t s_launchTestStarts = 0;
static uint32_t s_launchTestPathQueries = 0;
static uint32_t s_launchTestMonitorCloses = 0;
static uint32_t s_launchTestStartError = 0;
static std::string s_launchTestExecutable;
static std::vector<std::string> s_launchTestArguments;

std::vector<uint32_t> NetGatewayLaunchSystem::processPid(const std::string& processName)
{
    s_launchTestQueries.push_back(processName);
    return processName == "NetClientManager1.1.exe" ? s_launchTestMains :
        processName == "NetClientManagerGuard1.1.exe" ? s_launchTestGuards : std::vector<uint32_t>();
}

bool NetGatewayLaunchSystem::isProcessLeaseReady(const std::string& name, uint32_t pid)
{
    return name == "NetClientManager" ? s_launchTestMainReady &&
        std::find(s_launchTestMains.begin(), s_launchTestMains.end(), pid) != s_launchTestMains.end() :
        name == "NetClientManagerSupervisor" && s_launchTestGuardReady &&
        std::find(s_launchTestGuards.begin(), s_launchTestGuards.end(), pid) != s_launchTestGuards.end();
}

std::string NetGatewayLaunchSystem::currentExecutablePathUtf8()
{
    ++s_launchTestPathQueries;
    return "C:/suite/Product/Product.exe";
}

std::string NetGatewayLaunchSystem::GetName(const std::string& path, int32_t flag)
{
    (void)path;
    return flag == 4 ? "C:/suite/Product" : "";
}

std::string NetGatewayLaunchSystem::joinPath(const std::string& directory, const std::string& name)
{
    return directory + "/" + name;
}

std::wstring NetGatewayLaunchString::Utf8ToUnicode(const std::string& input)
{
    // 本探针仅提供ASCII固定夹具，不验证UTF8转换
    return std::wstring(input.begin(), input.end());
}

bool NetGatewayLaunchSystem::fileExists(const std::wstring& path)
{
    return (s_launchTestSibling && path == L"C:/suite/Product/../NetClientManager/NetClientManager1.1.exe") ||
        (s_launchTestFlat && path == L"C:/suite/Product/NetClientManager1.1.exe");
}

uint32_t NetGatewayLaunchSystem::startDetachedProcess(const std::string& executable, const std::vector<std::string>& arguments,
    uint32_t& pid, uint64_t& monitor)
{
    ++s_launchTestStarts;
    s_launchTestExecutable = executable;
    s_launchTestArguments = arguments;
    pid = s_launchTestStartError == 0 ? 99 : 0;
    monitor = s_launchTestStartError == 0 ? 900 : 0;
    return s_launchTestStartError;
}

void NetGatewayLaunchSystem::closeProcessMonitor(uint64_t& monitor)
{
    ++s_launchTestMonitorCloses;
    monitor = 0;
}

/** Windows启动策略探针，精确检查独立Guard文件名及不抢原配置；无真实OS副作用
@return 0通过，1失败
*/
int NetGatewayLaunchTest::check()
{
    s_launchTestMains.clear();
    s_launchTestGuards.clear();
    s_launchTestQueries.clear();
    s_launchTestMainReady = true;
    s_launchTestGuardReady = true;
    s_launchTestSibling = false;
    s_launchTestFlat = false;
    s_launchTestStarts = 0;
    s_launchTestPathQueries = 0;
    s_launchTestMonitorCloses = 0;
    s_launchTestStartError = 0;
    s_launchTestExecutable.clear();
    s_launchTestArguments.clear();
    uint32_t assertions = 0;
#define LAUNCH_CHECK(condition) { ++assertions; if (!(condition)) { std::printf("FAIL line %u: %s\n", static_cast<uint32_t>(__LINE__), #condition); return 1; } }
    uint32_t pid = 0;
    s_launchTestGuards.push_back(42);
    LAUNCH_CHECK(NetGatewayLaunchSubject::ensure(pid) == NI_OK && pid == 42);
    LAUNCH_CHECK(s_launchTestQueries.size() == 2 && s_launchTestQueries[0] == "NetClientManager1.1.exe" &&
        s_launchTestQueries[1] == "NetClientManagerGuard1.1.exe");
    LAUNCH_CHECK(s_launchTestStarts == 0 && s_launchTestPathQueries == 0 && s_launchTestMonitorCloses == 0);
    s_launchTestMains.push_back(7);
    s_launchTestQueries.clear();
    LAUNCH_CHECK(NetGatewayLaunchSubject::ensure(pid) == NI_OK && pid == 7);
    LAUNCH_CHECK(s_launchTestQueries.size() == 1 && s_launchTestStarts == 0);
    s_launchTestMains.push_back(8);
    LAUNCH_CHECK(NetGatewayLaunchSubject::ensure(pid) == NI_INVALID_STATE && pid == 0 && s_launchTestStarts == 0);
    s_launchTestMains.resize(1);
    s_launchTestMainReady = false;
    LAUNCH_CHECK(NetGatewayLaunchSubject::ensure(pid) == NI_OK && pid == 42 && s_launchTestStarts == 0);
    s_launchTestGuards.push_back(43);
    LAUNCH_CHECK(NetGatewayLaunchSubject::ensure(pid) == NI_INVALID_STATE && pid == 0 && s_launchTestStarts == 0);
    s_launchTestGuards.resize(1);
    s_launchTestGuardReady = false;
    s_launchTestSibling = true;
    s_launchTestFlat = true;
    LAUNCH_CHECK(NetGatewayLaunchSubject::ensure(pid) == NI_OK && pid == 99 && s_launchTestStarts == 1);
    LAUNCH_CHECK(s_launchTestExecutable == "C:/suite/Product/../NetClientManager/NetClientManager1.1.exe" &&
        s_launchTestArguments.size() == 1 && s_launchTestArguments[0] == "--background");
    LAUNCH_CHECK(s_launchTestMonitorCloses == 1 && s_launchTestPathQueries == 1);
    s_launchTestSibling = false;
    LAUNCH_CHECK(NetGatewayLaunchSubject::ensure(pid) == NI_OK && pid == 99 && s_launchTestStarts == 2 &&
        s_launchTestExecutable == "C:/suite/Product/NetClientManager1.1.exe");
    s_launchTestFlat = false;
    LAUNCH_CHECK(NetGatewayLaunchSubject::ensure(pid) == NI_NOT_FOUND && pid == 0 && s_launchTestStarts == 2);
    s_launchTestFlat = true;
    s_launchTestStartError = 5;
    LAUNCH_CHECK(NetGatewayLaunchSubject::ensure(pid) == NI_INVALID_STATE && pid == 0 && s_launchTestStarts == 3);
    LAUNCH_CHECK(s_launchTestMonitorCloses == 3);
    std::printf("Gateway launch policy assertions=%u PASS\n", assertions);
    return 0;
#undef LAUNCH_CHECK
}

void NetGatewayLaunchTest::run()
{
    if (check() != 0)
    {
        throw std::runtime_error("NetGatewayLaunchTest failed");
    }
}