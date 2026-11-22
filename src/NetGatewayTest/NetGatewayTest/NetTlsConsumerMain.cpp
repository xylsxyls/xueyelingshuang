#include "NetInterfaceTlsTestCases.h"
#include "NetCompactTest.h"
#include "NetCryptoTestCases.h"
#include <chrono>
#include <cstdio>
#include <stdexcept>

// 独立SDK消费进程的硬截止，不依赖生产库白盒入口。
static std::chrono::steady_clock::time_point g_tlsDeadline;

/** 检查TLS测试期限，构建执行器另有进程级超时
*/
void netTestCheckpoint()
{
    if (std::chrono::steady_clock::now() > g_tlsDeadline)
    {
        throw std::runtime_error("TLS consumer deadline");
    }
}

/** 使用公开TLS接口验证静态库及DLL的独立消费
@return 全部通过返回0，发生测试异常返回1
*/
int main()
{
    g_tlsDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(60);
    try
    {
        NetInterfaceTlsTestCases::handshakeAndClose();
        NetInterfaceTlsTestCases::certificateRejection();
        NetInterfaceTlsTestCases::tamperRejection();
        NetInterfaceTlsTestCases::replayRejection();
        NetInterfaceTlsTestCases::failures();
        NetInterfaceTlsTestCases::cApiAndEntropy();
        NetInterfaceTlsTestCases::boundedTransfer();
        NetInterfaceTlsTestCases::appendRecords();
        NetCryptoTestCases::derivationBoundary();
        NetCompactTest::run();
        NetCompactTest::tls();
        NetCompactTest::submission();
        std::puts("TLS/compact/submission public SDK cases=12 PASS");
        return 0;
    }
    catch (const std::exception& error)
    {
        std::printf("TLS public SDK FAIL: %s\n", error.what());
    }
    catch (...)
    {
        std::puts("TLS public SDK FAIL: unexpected exception");
    }
    return 1;
}