#include "NetTlsEntropyTest.h"
#include <stdexcept>
#include <algorithm>
#include "NetInterface/NetInterfaceAPI.h"
#include <cstdio>
#include <cstring>
#ifdef _DEBUG
#include <crtdbg.h>
#endif

/** 在隔离程序中启用；失败回调不得穿过C栈，全部创建失败资源须由本模块回收
@return 0全部检查通过，1断言或异常失败
*/
int NetTlsEntropyTest::check()
{
    uint32_t checks = 0;
    // condition为期望结果，message为静态说明；失败抛出仅由本测试main捕获的整数。
    const auto Check = [&checks](bool condition, const char* message)
    {
        ++checks;
        if (!condition)
        {
            std::printf("FAIL %s\n", message);
            throw 1;
        }
    };
    try
    {
#ifdef _DEBUG
        // 先初始化CRT诊断状态；会话生命周期全部在快照作用域内完成。
        _CrtMemState before;
        _CrtMemState after;
        _CrtMemCheckpoint(&before);
#endif
        {
            const uint8_t authority[] = {1};
            NetInterfaceTlsOptions options = {};
            options.authority = authority;
            options.authoritySize = sizeof(authority);
            options.hostname = "example.test";
            options.handshakeTimeoutMs = 1000;
            // 回调先写部分随机数再失败，验证任何失败路径都不会继续使用这些字节。
            // context借用uint32_t模式：0返回失败，1抛异常；output为size字节可写内存。
            options.entropy = [](void* context, uint8_t* output, uint32_t size) -> int32_t
            {
                std::memset(output, 0xa7, size / 2);
                if (*static_cast<uint32_t*>(context) != 0)
                {
                    throw 42;
                }
                return -1;
            };
            for (uint32_t mode = 0; mode < 2; ++mode)
            {
                options.entropyContext = &mode;
                for (uint32_t repeat = 0; repeat < 64; ++repeat)
                {
                    uint8_t bytes[48];
                    std::memset(bytes, 0xc3, sizeof(bytes));
                    Check(NetInterface::entropyBytes(options.entropy, &mode, bytes, sizeof(bytes)) == NI_INVALID_STATE,
                        "entropy callback failure did not fail closed");
                    for (size_t index = 0; index < sizeof(bytes); ++index)
                    {
                        Check(bytes[index] == 0, "failed entropy retained partial output");
                    }
                    NetInterfaceTls tls(options);
                    const int32_t failed = tls.status();
                    Check(failed == NET_INTERFACE_TLS_PROTOCOL && tls.detail() != 0,
                        "TLS constructor leaked entropy exception or lost failure");
                    uint32_t consumed = 77;
                    Check(tls.feed(bytes, sizeof(bytes), consumed) == failed && consumed == 0,
                        "failed TLS accepted input");
                    uint32_t accepted = 77;
                    Check(tls.write(bytes, sizeof(bytes), accepted) == failed && accepted == 0,
                        "failed TLS accepted plaintext");
                    uint32_t received = 77;
                    Check(tls.read(bytes, sizeof(bytes), received) == failed && received == 0,
                        "failed TLS delivered plaintext");
                    Check(tls.progress(1) == failed && tls.outputState() == failed && tls.finishInput() == failed && tls.close() == failed,
                        "failed TLS state was revived");
                    const uint8_t* output = reinterpret_cast<const uint8_t*>(1);
                    uint32_t size = 77;
                    Check(tls.peekOutput(output, size) == failed && output == nullptr && size == 0,
                        "failed initialization generated output");
                    NetInterfaceTlsHandle handle = reinterpret_cast<NetInterfaceTlsHandle>(1);
                    int32_t detail = 77;
                    Check(NetInterface::tlsCreate(&options, &handle, &detail) == failed && handle == nullptr && detail != 0,
                        "TLS C factory leaked exception or partial handle");
                    NetInterfaceConnection connection(options);
                    Check(connection.status() == failed && connection.detail() != 0,
                        "connection constructor lost entropy failure");
                    NetInterfaceConnectionHandle connectionHandle = reinterpret_cast<NetInterfaceConnectionHandle>(1);
                    detail = 77;
                    Check(NetInterface::connectionCreate(&options, &connectionHandle, &detail) == failed && connectionHandle == nullptr && detail != 0,
                        "connection C factory lost entropy failure");
                }
            }
            uint8_t bytes[8];
            std::memset(bytes, 0xa5, sizeof(bytes));
            Check(NetInterface::entropyBytes(nullptr, nullptr, bytes, sizeof(bytes)) == NI_INVALID_STATE,
                "missing entropy source accepted");
            for (size_t index = 0; index < sizeof(bytes); ++index)
            {
                Check(bytes[index] == 0, "missing entropy source retained output");
            }
            Check(NetInterface::entropyBytes(nullptr, nullptr, nullptr, 0) == NI_OK &&
                NetInterface::entropyBytes(nullptr, nullptr, nullptr, 1) == NI_INVALID_ARGUMENT &&
                NetInterface::entropyBytes(nullptr, nullptr, bytes, 8193) == NI_INVALID_ARGUMENT,
                "entropy argument boundaries changed");
        }
#ifdef _DEBUG
        _CrtMemCheckpoint(&after);
        Check(after.lCounts[_NORMAL_BLOCK] == before.lCounts[_NORMAL_BLOCK] &&
            after.lSizes[_NORMAL_BLOCK] == before.lSizes[_NORMAL_BLOCK], "failed TLS construction leaked CRT memory");
#endif
        std::printf("NetInterface TLS entropy boundary checks=%u PASS\n", checks);
        return 0;
    }
    catch (...)
    {
        std::printf("NetInterface TLS entropy boundary checks=%u FAIL\n", checks);
        return 1;
    }
}

void NetTlsEntropyTest::run()
{
    if (check() != 0)
    {
        throw std::runtime_error("NetTlsEntropyTest failed");
    }
}