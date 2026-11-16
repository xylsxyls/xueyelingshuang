#include "NetIsolatedTest.h"
#include "NetTestProcess.h"
#include "CSystem/CSystemAPI.h"
#include <atomic>
#include <cstdio>
#include <string>
#include <vector>

int32_t NetIsolatedTest::run(const char* argument, uint32_t timeoutMs)
{
    if (argument == nullptr || timeoutMs == 0)
    {
        return 98;
    }
    // 每次独占日志，不覆盖旧批次；子进程输出归并到父报告后才删除自己的临时文件。
    static std::atomic<uint32_t> s_sequence(0);
    const std::string executable = CSystem::currentExecutablePathUtf8();
    const std::string log = CSystem::joinPath(CSystem::GetName(executable, 4), "NetWorkTest-isolated-" +
        std::to_string(CSystem::currentProcessPid()) + "-" + std::to_string(s_sequence.fetch_add(1)) + ".log");
    int32_t result = 98;
    bool copied = false;
    {
        NetTestProcess process;
        if (!process.start(executable, std::vector<std::string>(1, argument), log))
        {
            return 98;
        }
        int32_t code = 98;
        if (process.wait(timeoutMs, code) == 1)
        {
            result = code;
#ifdef _WIN32
            if (process.waitTree(1000) != 1)
            {
                result = 98;
            }
#endif
        }
        else
        {
            process.terminate();
        }
        std::string output;
        if (process.output(output))
        {
#ifdef _WIN32
            // Windows文本stdout会把LF展开为CRLF，先消除子日志已有的CR，避免出现CRCRLF。
            for (size_t index = 0; (index = output.find("\r\n", index)) != std::string::npos; ++index)
            {
                output.erase(index, 1);
            }
#endif
            copied = std::fwrite(output.data(), 1, output.size(), stdout) == output.size() && std::fflush(stdout) == 0;
        }
    }
    if (!copied || !CSystem::deleteFile(log.c_str()))
    {
        result = 98;
        std::fprintf(stderr, "Isolated evidence could not be copied/removed: %s\n", log.c_str());
    }
    std::printf("ISOLATED %s exit=%d\n", argument, result);
    return result;
}