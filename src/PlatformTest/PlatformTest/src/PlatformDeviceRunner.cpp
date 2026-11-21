#include "PlatformTestHost.hpp"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <thread>

/** 组合固定的主机工具参数与本次调用参数，保留每个参数的边界
*/
static std::vector<std::string> Join(std::vector<std::string> base, const std::vector<std::string>& extra)
{
    base.insert(base.end(), extra.begin(), extra.end());
    return base;
}

/** 去掉工具输出首尾的空白，保留路径内部的空格
*/
static std::string Trim(const std::string& value)
{
    size_t first = value.find_first_not_of(" \t\r\n");
    return first == std::string::npos ? std::string() : value.substr(first, value.find_last_not_of(" \t\r\n") - first + 1);
}

/** 校验设备报告的 C++ 退出码，缺失或非数字状态按失败报告
*/
static int Status(const std::string& text)
{
    std::string value = Trim(text);
    if (value.empty() || value.size() > 3 || value.find_first_not_of("0123456789") != std::string::npos || std::atoi(value.c_str()) > 255)
    {
        throw std::runtime_error("Invalid device test exit status: " + text);
    }
    return std::atoi(value.c_str());
}

/** 生成一个 POSIX shell 字面参数，设备上的引号和展开符按原值传给 suite
*/
static std::string ShellArgument(const std::string& value)
{
    std::string quoted("'");
    for (size_t i = 0; i < value.size(); ++i)
    {
        if (value[i] == '\'')
        {
            quoted += "'\\''";
        }
        else
        {
            quoted += value[i];
        }
    }
    return quoted + "'";
}

/** 选择 SDK 中的 adb；未显式安装 SDK 时由系统 PATH 查找
*/
static std::string AndroidBridge()
{
    const char* home = std::getenv("ANDROID_HOME");
    if (!home || !*home)
    {
        home = std::getenv("ANDROID_SDK_ROOT");
    }
    if (home && *home)
    {
        std::string path = std::string(home) + "/platform-tools/adb";
#ifdef _WIN32
        path += ".exe";
#endif
        if (PlatformTestHost::exists(path))
        {
            return path;
        }
    }
    return "adb";
}

/** 部署 Android ELF 和运行库，读取设备实际返回的状态并清理本次目录
*/
static int Android(PlatformTestHost& host, const std::string& product, const std::string& device,
    const std::map<std::string, std::string>& libraries, const std::vector<std::string>& args)
{
    std::vector<std::string> adb(1, AndroidBridge());
    if (!device.empty())
    {
        adb = Join(adb, {"-s", device});
    }
    const std::string remote = "/data/local/tmp/platform-test-" + host.token();
    const std::string marker = "PLATFORM_TEST_RESULT_" + host.token() + "=";
    std::string output;
    host.command(Join(adb, {"shell", "mkdir -m 700 " + remote}), 90, output);
    int status = 1;
    try
    {
        host.command(Join(adb, {"push", product, remote + "/suite"}), 90, output);
        for (std::map<std::string, std::string>::const_iterator it = libraries.begin(); it != libraries.end(); ++it)
        {
            if (!PlatformTestHost::exists(it->second))
            {
                throw std::runtime_error("Missing Android runtime library " + it->second);
            }
            host.command(Join(adb, {"push", it->second, remote + "/" + it->first}), 90, output);
        }
        std::string command = "cd " + remote + " && chmod 700 suite && TMPDIR=$PWD SHARED_MEMORY_TEST_DIR=$PWD CSYSTEM_TEST_DIR=$PWD LD_LIBRARY_PATH=$PWD ./suite";
        for (size_t i = 0; i < args.size(); ++i)
        {
            command += " " + ShellArgument(args[i]);
        }
        command += "; code=$?; printf '\\n" + marker + "%s\\n' \"$code\"";
        host.command(Join(adb, {"shell", command}), 90, output);
        std::cout << output;
        size_t pos = output.rfind("\n" + marker);
        if (pos == std::string::npos)
        {
            throw std::runtime_error("Device disconnected before reporting the test status");
        }
        status = Status(output.substr(pos + 1 + marker.size()));
    }
    catch (...)
    {
        host.command(Join(adb, {"shell", "rm -rf " + remote}), 15, output, false);
        throw;
    }
    host.command(Join(adb, {"shell", "rm -rf " + remote}), 15, output);
    return status;
}

/** 部署 iOS 应用，从模拟器容器或配对设备复制该次测试的状态文件
*/
static int Apple(PlatformTestHost& host, bool simulator, const std::string& product, const std::string& device,
    const std::string& identifier, const std::vector<std::string>& args)
{
    if (device.empty() || identifier.empty())
    {
        throw std::runtime_error("An iOS device UDID and app bundle identifier are required");
    }
    const std::string name = "tmp/platform-test-" + host.token() + ".txt";
    std::vector<std::string> launch = Join({"--platform-test-run", host.token(), "--platform-test-arguments"}, args);
    std::string output;
    std::string statusPath = host.directory() + "/status.txt";
    if (simulator)
    {
        host.command({"xcrun", "simctl", "install", device, product}, 90, output);
        host.command({"xcrun", "simctl", "terminate", device, identifier}, 15, output, false);
        host.command(Join({"xcrun", "simctl", "launch", device, identifier}, launch), 90, output);
    }
    else
    {
        host.command({"xcrun", "devicectl", "device", "install", "app", "--device", device, product}, 90, output);
        host.command(Join({"xcrun", "devicectl", "device", "process", "launch", "--device", device, "--terminate-existing", identifier, "--"}, launch), 90, output);
    }
    int status = 1;
    try
    {
        if (simulator)
        {
            host.command({"xcrun", "simctl", "get_app_container", device, identifier, "data"}, 15, output);
            std::string folder = Trim(output);
            if (folder.empty() || !host.exists(folder))
            {
                throw std::runtime_error("Invalid simulator data container " + folder);
            }
            statusPath = folder + "/" + name;
        }
        const std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() + std::chrono::seconds(90);
        for (;;)
        {
            bool copied = simulator || host.command({"xcrun", "devicectl", "device", "copy", "from", "--device", device, "--domain-type", "appDataContainer", "--domain-identifier", identifier, "--source", name, "--destination", statusPath}, 15, output, false) == 0;
            if (copied && host.exists(statusPath))
            {
                const std::string result = host.read(statusPath);
                const size_t newline = result.find('\n');
                status = Status(result.substr(0, newline));
                if (newline != std::string::npos)
                {
                    std::cout << result.substr(newline + 1);
                }
                break;
            }
            if (std::chrono::steady_clock::now() >= deadline)
            {
                throw std::runtime_error("Timed out waiting for the iOS test: " + output);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(simulator ? 300 : 1000));
        }
    }
    catch (...)
    {
        if (simulator)
        {
            host.command({"xcrun", "simctl", "terminate", device, identifier}, 15, output, false);
            std::remove(statusPath.c_str());
        }
        throw;
    }
    if (simulator)
    {
        host.command({"xcrun", "simctl", "terminate", device, identifier}, 15, output, false);
        std::remove(statusPath.c_str());
    }
    return status;
}

/** 使用本次独占的 adb reverse 映射执行回环 HTTP，清理时确认映射仍属于该测试
*/
static int AndroidLoopback(PlatformTestHost& host, const std::string& product, const std::string& device,
    const std::map<std::string, std::string>& libraries, const std::vector<std::string>& args, const std::string& port)
{
    if (port.empty() || port.size() > 5 || port.find_first_not_of("0123456789") != std::string::npos || std::atoi(port.c_str()) < 1 || std::atoi(port.c_str()) > 65535)
    {
        throw std::runtime_error("Invalid loopback port " + port);
    }
    std::vector<std::string> adb(1, AndroidBridge());
    std::string output;
    if (!device.empty())
    {
        adb = Join(adb, {"-s", device});
    }
    host.command(Join(adb, {"get-serialno"}), 15, output);
    const std::string serial = Trim(output);
    if (serial.empty() || serial == "unknown")
    {
        throw std::runtime_error("adb did not identify a connected device");
    }
    const std::vector<std::string> reverse = {AndroidBridge(), "-s", serial, "reverse"};
    const std::string endpoint = "tcp:" + port;
    host.command(Join(reverse, {"--no-rebind", endpoint, endpoint}), 15, output);
    // 仅回收仍映射到本次主机端口的条目，用户或其他部署进程可能改写过映射。
    auto cleanup = [&]() {
        host.command(Join(reverse, {"--list"}), 15, output);
        std::istringstream mappings(output);
        std::string line;
        while (std::getline(mappings, line))
        {
            std::istringstream columns(line);
            std::string transport, from, to;
            if ((columns >> transport >> from >> to) && from == endpoint && to == endpoint)
            {
                host.command(Join(reverse, {"--remove", endpoint}), 15, output);
                break;
            }
        }
    };
    int code = 1;
    try
    {
        code = Android(host, product, serial, libraries, args);
    }
    catch (...)
    {
        cleanup();
        throw;
    }
    cleanup();
    return code;
}

int main(int argc, char** argv)
{
    try
    {
        if (argc < 3 || !PlatformTestHost::exists(argv[2]))
        {
            throw std::runtime_error("Usage: PlatformDeviceRunner android|ios-simulator|ios-device <build product> [options] -- [arguments]");
        }
        const std::string mode(argv[1]);
        std::string device = mode == "ios-simulator" ? "booted" : "";
        std::string identifier;
        std::string sdk;
        std::string loopbackPort;
        std::map<std::string, std::string> libraries;
        std::vector<std::string> arguments;
        for (int i = 3; i < argc; ++i)
        {
            const std::string key(argv[i]);
            if (key == "--")
            {
                arguments.assign(argv + i + 1, argv + argc);
                break;
            }
            if (++i == argc)
            {
                throw std::runtime_error("Missing value for " + key);
            }
            const std::string value(argv[i]);
            if (key == "--device")
            {
                device = value;
            }
            else if (key == "--identifier")
            {
                identifier = value;
            }
            else if (key == "--sdk-lib")
            {
                sdk = value;
            }
            else if (key == "--http-loopback")
            {
                loopbackPort = value;
            }
            else if (key == "--runtime")
            {
                libraries[value.substr(value.find_last_of("/\\") + 1)] = value;
            }
            else
            {
                throw std::runtime_error("Unknown option " + key);
            }
        }
        PlatformTestHost host;
        if (mode == "android")
        {
            const std::vector<std::string> files = PlatformTestHost::files(sdk);
            for (size_t i = 0; !sdk.empty() && i < files.size(); ++i)
            {
                const std::string& name = files[i];
                if (name.size() > 3 && name.compare(name.size() - 3, 3, ".so") == 0)
                {
                    libraries.insert(std::make_pair(name, sdk + "/" + name));
                }
            }
            return loopbackPort.empty() ? Android(host, argv[2], device, libraries, arguments) :
                AndroidLoopback(host, argv[2], device, libraries, arguments, loopbackPort);
        }
        if (mode == "ios-device" || mode == "ios-simulator")
        {
            return Apple(host, mode == "ios-simulator", argv[2], device, identifier, arguments);
        }
        throw std::runtime_error("Unknown device platform " + mode);
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}