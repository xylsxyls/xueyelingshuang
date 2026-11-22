#include "NetSenderFacadeNetSenderCppTest.h"
#include "NetSenderFacadeNetPeerCppTest.h"
#include "NetSenderFacadeNetPeerCTest.h"
#include "NetSenderFacadeNetWorkCppTest.h"
#include "NetSenderFacadeNetWorkCTest.h"
#include <chrono>
#include <cstdio>
#include <stdexcept>

// DLL与静态库共用公开契约用例；不借助导出内部实现维持测试链接
static std::chrono::steady_clock::time_point g_sdkDeadline;

/** 检查单个公开接口用例的墙钟期限，进程外另设硬超时
*/
void netTestCheckpoint()
{
    if (std::chrono::steady_clock::now() > g_sdkDeadline)
    {
        throw std::runtime_error("SDK consumer deadline");
    }
}

/** 在独立消费进程执行三种发送门面的C++及两种C包装契约
@return 全部通过返回0，任一边界或清理失败返回1
*/
int main()
{
    const char* names[] = {"peerCpp", "peerC"
#ifndef NET_TEST_PEER_ONLY
        , "desktopCpp", "networkCpp", "networkC"
#endif
    };
    void (*cases[])() = {&NetSenderFacadeNetPeerCppTest::peerCpp, &NetSenderFacadeNetPeerCTest::peerC
#ifndef NET_TEST_PEER_ONLY
        , &NetSenderFacadeNetSenderCppTest::desktopCpp,
        &NetSenderFacadeNetWorkCppTest::workCpp, &NetSenderFacadeNetWorkCTest::workC
#endif
    };
    unsigned failed = 0;
    for (unsigned index = 0; index < sizeof(cases) / sizeof(cases[0]); ++index)
    {
        g_sdkDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(60);
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
    std::printf("SDK consumers=%u failed=%u\n", static_cast<unsigned>(sizeof(cases) / sizeof(cases[0])), failed);
    return failed == 0 ? 0 : 1;
}