#include "NetServerManagerTest.h"
#include "CSystem/CSystemAPI.h"
#include <string>
#include <vector>
#include "PlatformTest/PlatformTestAPI.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>

#define CHECK_TEST(expr) do { if (!(expr)) { std::fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #expr); return 1; } } while (false)

/** 调用同目录的管理进程，分别传递参数并限制单次运行时间
@param [in] arguments 命令行选项
@param [out] output 标准输出和错误输出
@return 进程退出码，启动或超时错误为1
*/
static int Run(const std::vector<std::string>& arguments, std::string& output)
{
    std::string program = CSystem::currentExecutablePathUtf8();
    program.erase(program.find_last_of("/\\") + 1);
    program += "NetServerManager1.1";
#ifdef _WIN32
    program += ".exe";
#endif
    std::vector<std::string> command(1, program);
    command.insert(command.end(), arguments.begin(), arguments.end());
    const int code = PlatformTest::runDesktopProcess(command, 30, output);
    std::printf("%s", output.c_str());
    return code;
}

/** 验证真正执行的程序公开了预期的命令行配置
@return 全部通过为0
*/
static int Help()
{
    std::string output;
    CHECK_TEST(Run({"--help"}, output) == 0 && output.find("NetServerManager") != std::string::npos &&
        output.find("--protocol=3") != std::string::npos && output.find("--identity-directory") != std::string::npos);
    return 0;
}

/** 检查参数验证不会误入服务启动路径
@return 全部通过为0
*/
static int Reject()
{
    std::string text;
    CHECK_TEST(Run({"--legacy"}, text) == 2 && text.find("Legacy gateway mode has been removed") != std::string::npos);
    CHECK_TEST(Run({"--certificate=unused"}, text) == 2 && text.find("Invalid identity configuration") != std::string::npos);
    CHECK_TEST(Run({"--port=65536"}, text) == 2 && text.find("Invalid gateway configuration") != std::string::npos);
    return 0;
}

/** 只在回环地址短时运行本次进程，输出目录使用独占 UUID，完成后清理
@return 看到本次 READY、STOPPED 且清理完成为0
*/
static int Lifetime()
{
    const std::string root = CSystem::GetEnvironment("XUEYELINGSHUANG");
    const std::string token = CSystem::uuid(0);
    CHECK_TEST(!token.empty());
    const std::string directory = CSystem::joinPath(root.empty() ? "." : CSystem::joinPath(root, "src/temp"), "NetServerManagerTest-" + token);
    CHECK_TEST(CSystem::CreateDir(directory));
    const std::string cache = CSystem::joinPath(directory, "nodes.cache");
    std::string text;
    const int code = Run({"--protocol=3", "--listen=127.0.0.1", "--advertise=127.0.0.1", "--port=0", "--identity-directory=" + directory, "--run-for-ms=700"}, text);
    const size_t ready = text.find("GATEWAY_READY port=");
    const size_t stopped = text.find("GATEWAY_STOPPED");
    const std::string identity = CSystem::joinPath(directory, "identity.dat");
    const bool generated = CSystem::DirOrFileExist(identity);
    if (generated)
    {
        CSystem::deleteFile(identity.c_str());
    }
    if (CSystem::DirOrFileExist(cache))
    {
        CSystem::deleteFile(cache.c_str());
    }
    CHECK_TEST(CSystem::DestroyDir(directory));
    CHECK_TEST(code == 0 && generated && ready != std::string::npos && stopped != std::string::npos && ready < stopped);
    return 0;
}

/** 按稳定编号选择用例，返回库测试调度器的退出码
@param [in] argc 参数个数
@param [in] argv 命令行参数
@return 全部通过为0，断言失败为1，参数错误为2
*/
int main(int argc, char** argv)
try
{
    const LibraryTestCase cases[] = {
        {1, false, Help},
        {2, false, Reject},
        {3, true, Lifetime}
    };
    return LibraryCaseSelection::run(argc, argv, cases);
}
catch (const std::exception& error)
{
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}