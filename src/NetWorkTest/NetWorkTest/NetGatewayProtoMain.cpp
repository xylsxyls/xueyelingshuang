#include "NetInterfaceProtoTestCases.h"
#include <cstdio>
#include <exception>
#include <stdexcept>
#include <chrono>

// 独立入口复用正式网关用例；不调用网络、不加载旧ProtoMessage库。
// 每个用例60秒期限，外部构建驱动另设进程超时；完整入口仍由NetWorkTest执行。
static std::chrono::steady_clock::time_point g_protoDeadline;

/** 检查当前用例期限，异常由本测试入口收集，不修改被测库
*/
void netTestCheckpoint()
{
    if (std::chrono::steady_clock::now() > g_protoDeadline)
    {
        throw std::runtime_error("protobuf regression deadline");
    }
}

/** 逐项执行共享用例并输出真实结果
@return 所有选择用例通过为0，否则为1
*/
int main()
{
    const char* names[] = {"wireCompatibility", "invalidValues", "ownership", "limits",
        "atomicContainers", "initializationRecovery"};
    void (*cases[])() = {&NetInterfaceProtoTestCases::wireCompatibility,
        &NetInterfaceProtoTestCases::invalidValues, &NetInterfaceProtoTestCases::ownership,
        &NetInterfaceProtoTestCases::limits, &NetInterfaceProtoTestCases::atomicContainers,
        &NetInterfaceProtoTestCases::initializationRecovery};
    unsigned failed = 0;
    for (unsigned index = 0; index < sizeof(cases) / sizeof(cases[0]); ++index)
    {
        g_protoDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(60);
        try
        {
            cases[index]();
            std::printf("PASS %s\n", names[index]);
        }
        catch (const std::exception& error)
        {
            ++failed;
            std::printf("FAIL %s: %s\n", names[index], error.what());
        }
        catch (...)
        {
            ++failed;
            std::printf("FAIL %s: unexpected exception\n", names[index]);
        }
    }
    std::printf("gateway protobuf cases=6 failed=%u\n", failed);
    return failed == 0 ? 0 : 1;
}