#include "NetClientGatewayWatchdog.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "CDump/CDumpAPI.h"
#include <cstdio>
#ifdef _WIN32
#include <windows.h>

/** 独立守护入口，只监视同目录正式网关，不创建ProcessWork收件箱
@param [in] argc 原平台参数数量
@param [in] argv 原平台参数，Windows仍由CSystem读取Unicode命令行
@return 正常退出0，监视失败1，非法内部调用2
*/
int main(int argc, char** argv)
{
    if (!CSystem::bindInheritedOutput())
    {
        return 1;
    }
    CDump::declareDumpFile();
    try
    {
        std::vector<std::string> arguments;
        if (!CSystem::commandArguments(argc, argv, arguments) || arguments.empty())
        {
            return 2;
        }
        const std::string prefix = "--watch-parent=";
        if (arguments.front().compare(0, prefix.size(), prefix) != 0)
        {
            return 2;
        }
        const std::string text = arguments.front().substr(prefix.size());
        if (text.empty() || text.size() > 10 ||
            text.find_first_not_of("0123456789") != std::string::npos)
        {
            return 2;
        }
        uint64_t parent = 0;
        for (char byte : text)
        {
            parent = parent * 10 + byte - '0';
        }
        if (parent == 0 || parent > 0x7fffffff)
        {
            return 2;
        }
        arguments.erase(arguments.begin());
        for (const std::string& argument : arguments)
        {
            if (argument == "--replace" || argument == "--supervised" ||
                argument.compare(0, prefix.size(), prefix) == 0)
            {
                return 2;
            }
        }
        const std::string executable = CSystem::joinPath(CSystem::GetName(CSystem::currentExecutablePathUtf8(), 4),
            "NetClientManager1.1.exe");
        if (!CSystem::fileExists(CStringManager::Utf8ToUnicode(executable)))
        {
            return 1;
        }
        return NetClientGatewayWatchdog::run(static_cast<uint32_t>(parent), executable, arguments);
    }
    catch (...)
    {
        std::fputs("CLIENT_GATEWAY_WATCHDOG_FAILED: startup or monitoring exception\n", stderr);
        return 1;
    }
}

#endif