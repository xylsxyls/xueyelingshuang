#ifdef NET_TEST_IPC
#include "NetSenderLocalHubTest.h"
#include "NetIpcAdapterTest.h"
#endif
#include "NetCryptoTestCases.h"
#include "NetInterfaceProtoTestCases.h"
#include "NetGatewaySuiteCases.h"
#ifdef NET_TEST_SUBMISSION
#include "NetSubmissionConcurrentTest.h"
#include "NetSubmissionTestCases.h"
#endif
#ifdef NET_TEST_NATIVE_FILE
#include "NetNativeFileTestCases.h"
#endif
#ifdef NET_TEST_HOST_BRIDGE
#include "NetWorkHostBridgeTest.h"
#endif
#include <cstdio>
#include <chrono>
#include <stdexcept>

// 无socket的协议模块回归，复用NetGatewayTest正式用例，不代替多进程整体测试。
static std::chrono::steady_clock::time_point g_interfaceDeadline;

/** 检查本用例运行期限，外层再以子进程超时兜底
*/
void netTestCheckpoint()
{
    if (std::chrono::steady_clock::now() > g_interfaceDeadline)
    {
        throw std::runtime_error("interface case deadline");
    }
}

/** 运行迁移后的消息、摘要及组包边界
@return 全部通过返回0，否则返回1
*/
int main()
{
    const char* names[] = {"hashBoundary", "derivationBoundary", "wireCompatibility", "invalidValues", "ownership",
        "messageBoundary", "limits", "atomicContainers", "streamingHash", "wireVector", "fragmentation", "backpressure",
        "invalidHeaders", "sizeLimits", "endOfInput", "arguments", "encodingAliases", "fileFlowBasic", "fileFlowSnapshot"
#ifdef NET_TEST_SUBMISSION
        , "submissionConcurrency", "submissionBoundary", "submissionBenchmark"
#endif
#ifdef NET_TEST_NATIVE_FILE
        , "nativeFileTemporary", "nativeFileCleanup"
#endif
#ifdef NET_TEST_HOST_BRIDGE
        , "hostBridge"
#endif
#ifdef NET_TEST_IPC
        , "ipcCapacity", "ipcPublication", "ipcLifecycle", "ipcPhase", "ipcConcurrent", "ipcConcurrentStop"
#endif
    };
    void (*cases[])() = {&NetCryptoTestCases::hashBoundary, &NetCryptoTestCases::derivationBoundary,
        &NetInterfaceProtoTestCases::wireCompatibility, &NetInterfaceProtoTestCases::invalidValues,
        &NetInterfaceProtoTestCases::ownership, &NetInterfaceProtoTestCases::cBoundary,
        &NetInterfaceProtoTestCases::limits, &NetInterfaceProtoTestCases::atomicContainers,
        &NetGatewaySuiteCases::streamingHash, &NetGatewaySuiteCases::wireVector, &NetGatewaySuiteCases::fragmentation,
        &NetGatewaySuiteCases::backpressure, &NetGatewaySuiteCases::invalidHeaders, &NetGatewaySuiteCases::sizeLimits,
        &NetGatewaySuiteCases::endOfInput, &NetGatewaySuiteCases::arguments, &NetGatewaySuiteCases::encodingAliases,
        &NetGatewaySuiteCases::fileFlowBasic, &NetGatewaySuiteCases::fileFlowSnapshot
#ifdef NET_TEST_SUBMISSION
        , &NetSubmissionConcurrentTest::run, &NetSubmissionTestCases::boundary, &NetSubmissionTestCases::benchmark
#endif
#ifdef NET_TEST_NATIVE_FILE
        , &NetNativeFileTestCases::temporary, &NetNativeFileTestCases::cleanup
#endif
#ifdef NET_TEST_HOST_BRIDGE
        , &NetWorkHostBridgeTest::run
#endif
#ifdef NET_TEST_IPC
        , &NetIpcAdapterTest::capacity, &NetIpcAdapterTest::publication, &NetIpcAdapterTest::lifecycle, &NetIpcAdapterTest::phase, &NetIpcAdapterTest::concurrentInbox, &NetIpcAdapterTest::concurrentStop
#endif
    };
    unsigned failed = 0;
#ifdef NET_TEST_IPC
    if (NetSenderLocalHubTest::run() != 0) { ++failed; }
#endif
    for (unsigned index = 0; index < sizeof(cases) / sizeof(cases[0]); ++index)
    {
        g_interfaceDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(60);
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
    std::printf("interface cases=%u failed=%u\n", static_cast<unsigned>(sizeof(cases) / sizeof(cases[0])), failed);
    return failed == 0 ? 0 : 1;
}