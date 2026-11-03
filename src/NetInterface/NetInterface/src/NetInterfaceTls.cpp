#include "NetInterfaceMacro.h"
#include "NetInterfaceTls.h"
#include "NetInterfaceTlsState.h"
#include "NetInterfaceTypes.h"
#include <algorithm>
#include <cstring>
#include <new>

NetInterfaceTls::NetInterfaceTls(const NetInterfaceTlsOptions& options) :
m_state(new (std::nothrow) NetInterfaceTlsState())
{
    if (m_state != nullptr)
    {
        m_state->configure(options);
    }
}

NetInterfaceTls::~NetInterfaceTls()
{
    delete m_state;
}

int32_t NetInterfaceTls::feed(const uint8_t* data, uint32_t size, uint32_t& consumed)
{
    consumed = 0;
    if (status() < 0)
    {
        return status();
    }
    if (size != 0 && data == nullptr)
    {
        return NET_INTERFACE_TLS_ARGUMENT;
    }
    if (m_state->m_inputFinished || m_state->m_status == NET_INTERFACE_TLS_CLOSED)
    {
        return NET_INTERFACE_TLS_STATE;
    }
    consumed = m_state->m_input.append(data, size);
    return consumed == size ? NET_INTERFACE_TLS_OK : NET_INTERFACE_TLS_WANT_IO;
}

int32_t NetInterfaceTls::progress(uint64_t monotonicMs)
{
    if (status() < 0 || status() == NET_INTERFACE_TLS_CLOSED)
    {
        return status();
    }
    if (!m_state->m_clockStarted)
    {
        m_state->m_clockStarted = true;
        m_state->m_startedMs = monotonicMs;
    }
    else if (monotonicMs < m_state->m_lastMs)
    {
        return m_state->fail(NET_INTERFACE_TLS_ARGUMENT, 0);
    }
    m_state->m_lastMs = monotonicMs;
    if (m_state->m_status == NET_INTERFACE_TLS_HANDSHAKE)
    {
        if (monotonicMs - m_state->m_startedMs >= m_state->m_timeoutMs)
        {
            return m_state->fail(NET_INTERFACE_TLS_TIMEOUT, 0);
        }
        int result = mbedtls_ssl_handshake(&m_state->m_ssl);
        if (result == MBEDTLS_ERR_SSL_WANT_READ || result == MBEDTLS_ERR_SSL_WANT_WRITE)
        {
            return NET_INTERFACE_TLS_HANDSHAKE;
        }
        if (result != 0)
        {
            return m_state->fail(m_state->m_inputFinished ? NET_INTERFACE_TLS_TRUNCATED : NET_INTERFACE_TLS_PROTOCOL, result);
        }
        m_state->m_status = NET_INTERFACE_TLS_READY;
    }
    int32_t result = m_state->flush();
    return result < 0 ? result : m_state->m_status;
}

int32_t NetInterfaceTls::peekOutput(const uint8_t*& data, uint32_t& size) const
{
    data = nullptr;
    size = 0;
    if (m_state == nullptr)
    {
        return NET_INTERFACE_TLS_MEMORY;
    }
    size = m_state->m_output.peek(data);
    return m_state->m_status < 0 ? m_state->m_status : NET_INTERFACE_TLS_OK;
}

int32_t NetInterfaceTls::consumeOutput(uint32_t size)
{
    if (m_state == nullptr)
    {
        return NET_INTERFACE_TLS_MEMORY;
    }
    return m_state->m_output.consume(size) ? NET_INTERFACE_TLS_OK : NET_INTERFACE_TLS_ARGUMENT;
}

int32_t NetInterfaceTls::write(const uint8_t* data, uint32_t size, uint32_t& accepted)
{
    accepted = 0;
    if (status() < 0)
    {
        return status();
    }
    if (status() != NET_INTERFACE_TLS_READY || m_state->m_closeStarted)
    {
        return NET_INTERFACE_TLS_STATE;
    }
    if (size != 0 && data == nullptr)
    {
        return NET_INTERFACE_TLS_ARGUMENT;
    }
    if (m_state->m_plaintextSize != 0)
    {
        return NET_INTERFACE_TLS_WANT_IO;
    }
    accepted = (std::min)(size, static_cast<uint32_t>(sizeof(m_state->m_plaintext)));
    if (accepted != 0)
    {
        std::memcpy(m_state->m_plaintext, data, accepted);
        m_state->m_plaintextSize = accepted;
    }
    // 接受只表示复制入队；progress负责转换，避免返回失败却已消费的歧义
    return NET_INTERFACE_TLS_OK;
}

int32_t NetInterfaceTls::read(uint8_t* output, uint32_t capacity, uint32_t& received)
{
    received = 0;
    if (status() < 0 || status() == NET_INTERFACE_TLS_CLOSED)
    {
        return status();
    }
    if (status() != NET_INTERFACE_TLS_READY)
    {
        return NET_INTERFACE_TLS_STATE;
    }
    if (capacity != 0 && output == nullptr)
    {
        return NET_INTERFACE_TLS_ARGUMENT;
    }
    if (capacity == 0)
    {
        return NET_INTERFACE_TLS_OK;
    }
    int result = mbedtls_ssl_read(&m_state->m_ssl, output, (std::min)(capacity, static_cast<uint32_t>(16384)));
    if (result == MBEDTLS_ERR_SSL_WANT_READ || result == MBEDTLS_ERR_SSL_WANT_WRITE)
    {
        return NET_INTERFACE_TLS_WANT_IO;
    }
    if (result == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY)
    {
        m_state->m_status = NET_INTERFACE_TLS_CLOSED;
        return m_state->m_status;
    }
    if (result <= 0)
    {
        return m_state->fail(result == 0 || m_state->m_inputFinished ? NET_INTERFACE_TLS_TRUNCATED : NET_INTERFACE_TLS_PROTOCOL, result);
    }
    received = static_cast<uint32_t>(result);
    return NET_INTERFACE_TLS_OK;
}

int32_t NetInterfaceTls::finishInput()
{
    if (status() < 0)
    {
        return status();
    }
    m_state->m_inputFinished = true;
    return NET_INTERFACE_TLS_OK;
}

int32_t NetInterfaceTls::close()
{
    if (status() < 0)
    {
        return status();
    }
    if (status() == NET_INTERFACE_TLS_HANDSHAKE)
    {
        return NET_INTERFACE_TLS_STATE;
    }
    m_state->m_closeStarted = true;
    if (m_state->m_closeSent)
    {
        return NET_INTERFACE_TLS_OK;
    }
    int32_t flushed = m_state->flush();
    if (flushed != NET_INTERFACE_TLS_OK)
    {
        return flushed;
    }
    int result = mbedtls_ssl_close_notify(&m_state->m_ssl);
    if (result == MBEDTLS_ERR_SSL_WANT_READ || result == MBEDTLS_ERR_SSL_WANT_WRITE)
    {
        return NET_INTERFACE_TLS_WANT_IO;
    }
    if (result != 0)
    {
        return m_state->fail(NET_INTERFACE_TLS_PROTOCOL, result);
    }
    m_state->m_closeSent = true;
    return NET_INTERFACE_TLS_OK;
}

int32_t NetInterfaceTls::status() const
{
    return m_state == nullptr ? NET_INTERFACE_TLS_MEMORY : m_state->m_status;
}

int32_t NetInterfaceTls::detail() const
{
    return m_state == nullptr ? 0 : m_state->m_detail;
}

int32_t NetInterfaceTls::outputState() const
{
    const int32_t current = status();
    if (current < 0) { return current; }
    const uint8_t* bytes = nullptr;
    return m_state->m_plaintextSize != m_state->m_plaintextOffset || m_state->m_output.peek(bytes) != 0 ? NI_READY : NI_OK;
}

// #include <cstdio>
// #ifdef _WIN32
// #include <windows.h>
// #endif
//
// /** 仅在隔离单测main中替换标准nothrow分配；生产编译保持整段关闭
// @param [in] size 本单测不分配内存
// @param [in] tag 标准nothrow标记
// @return 始终空指针，验证实际构造失败路径
// */
// void* operator new(std::size_t size, const std::nothrow_t& tag) throw()
// {
//     (void)size;
//     (void)tag;
//     return nullptr;
// }
//
// /** 放开本段后独立编译测试，测完恢复注释；不与应用main同时链接
// @return 0全部错误查询安全，1错误语义不符
// */
// int main()
// {
// #ifdef _WIN32
//     SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
// #endif
//     NetInterfaceTlsOptions options = {};
//     NetInterfaceTls tls(options);
//     const uint8_t* output = reinterpret_cast<const uint8_t*>(1);
//     uint32_t size = 7;
//     uint32_t accepted = 7;
//     uint32_t received = 7;
//     uint32_t consumed = 7;
//     const bool passed = tls.status() == NET_INTERFACE_TLS_MEMORY && tls.detail() == 0 &&
//         tls.feed(nullptr, 0, consumed) == NET_INTERFACE_TLS_MEMORY && consumed == 0 &&
//         tls.progress(1) == NET_INTERFACE_TLS_MEMORY &&
//         tls.write(nullptr, 0, accepted) == NET_INTERFACE_TLS_MEMORY && accepted == 0 &&
//         tls.read(nullptr, 0, received) == NET_INTERFACE_TLS_MEMORY && received == 0 &&
//         tls.peekOutput(output, size) == NET_INTERFACE_TLS_MEMORY && output == nullptr && size == 0 &&
//         tls.consumeOutput(0) == NET_INTERFACE_TLS_MEMORY &&
//         tls.finishInput() == NET_INTERFACE_TLS_MEMORY && tls.close() == NET_INTERFACE_TLS_MEMORY &&
//         tls.outputState() == NET_INTERFACE_TLS_MEMORY;
//     std::printf("TLS_ALLOCATION_FAILURE %s\n", passed ? "PASS" : "FAIL");
//     return passed ? 0 : 1;
// }

// NET_INTERFACE_TLS_ENTROPY_UNIT_BEGIN
//#include "NetInterface.h"
//#include <cstdio>
//#include <cstring>
//#ifdef _DEBUG
//#include <crtdbg.h>
//#endif
//
///** 在隔离程序中启用；失败回调不得穿过C栈，全部创建失败资源须由本模块回收
//@return 0全部检查通过，1断言或异常失败
//*/
//int main()
//{
//    uint32_t checks = 0;
//    // condition为期望结果，message为静态说明；失败抛出仅由本测试main捕获的整数。
//    const auto Check = [&checks](bool condition, const char* message)
//    {
//        ++checks;
//        if (!condition)
//        {
//            std::printf("FAIL %s\n", message);
//            throw 1;
//        }
//    };
//    try
//    {
//#ifdef _DEBUG
//        // 先初始化CRT诊断状态；会话生命周期全部在快照作用域内完成。
//        _CrtMemState before;
//        _CrtMemState after;
//        _CrtMemCheckpoint(&before);
//#endif
//        {
//            const uint8_t authority[] = {1};
//            NetInterfaceTlsOptions options = {};
//            options.authority = authority;
//            options.authoritySize = sizeof(authority);
//            options.hostname = "example.test";
//            options.handshakeTimeoutMs = 1000;
//            // 回调先写部分随机数再失败，验证任何失败路径都不会继续使用这些字节。
//            // context借用uint32_t模式：0返回失败，1抛异常；output为size字节可写内存。
//            options.entropy = [](void* context, uint8_t* output, uint32_t size) -> int32_t
//            {
//                std::memset(output, 0xa7, size / 2);
//                if (*static_cast<uint32_t*>(context) != 0)
//                {
//                    throw 42;
//                }
//                return -1;
//            };
//            for (uint32_t mode = 0; mode < 2; ++mode)
//            {
//                options.entropyContext = &mode;
//                for (uint32_t repeat = 0; repeat < 64; ++repeat)
//                {
//                    uint8_t bytes[48];
//                    std::memset(bytes, 0xc3, sizeof(bytes));
//                    Check(NetInterface::entropyBytes(options.entropy, &mode, bytes, sizeof(bytes)) == NI_INVALID_STATE,
//                        "entropy callback failure did not fail closed");
//                    for (size_t index = 0; index < sizeof(bytes); ++index)
//                    {
//                        Check(bytes[index] == 0, "failed entropy retained partial output");
//                    }
//                    NetInterfaceTls tls(options);
//                    const int32_t failed = tls.status();
//                    Check(failed == NET_INTERFACE_TLS_PROTOCOL && tls.detail() != 0,
//                        "TLS constructor leaked entropy exception or lost failure");
//                    uint32_t consumed = 77;
//                    Check(tls.feed(bytes, sizeof(bytes), consumed) == failed && consumed == 0,
//                        "failed TLS accepted input");
//                    uint32_t accepted = 77;
//                    Check(tls.write(bytes, sizeof(bytes), accepted) == failed && accepted == 0,
//                        "failed TLS accepted plaintext");
//                    uint32_t received = 77;
//                    Check(tls.read(bytes, sizeof(bytes), received) == failed && received == 0,
//                        "failed TLS delivered plaintext");
//                    Check(tls.progress(1) == failed && tls.outputState() == failed && tls.finishInput() == failed && tls.close() == failed,
//                        "failed TLS state was revived");
//                    const uint8_t* output = reinterpret_cast<const uint8_t*>(1);
//                    uint32_t size = 77;
//                    Check(tls.peekOutput(output, size) == failed && output == nullptr && size == 0,
//                        "failed initialization generated output");
//                    NetInterfaceTlsHandle handle = reinterpret_cast<NetInterfaceTlsHandle>(1);
//                    int32_t detail = 77;
//                    Check(NetInterface::tlsCreate(&options, &handle, &detail) == failed && handle == nullptr && detail != 0,
//                        "TLS C factory leaked exception or partial handle");
//                    NetInterfaceConnection connection(options);
//                    Check(connection.status() == failed && connection.detail() != 0,
//                        "connection constructor lost entropy failure");
//                    NetInterfaceConnectionHandle connectionHandle = reinterpret_cast<NetInterfaceConnectionHandle>(1);
//                    detail = 77;
//                    Check(NetInterface::connectionCreate(&options, &connectionHandle, &detail) == failed && connectionHandle == nullptr && detail != 0,
//                        "connection C factory lost entropy failure");
//                }
//            }
//            uint8_t bytes[8];
//            std::memset(bytes, 0xa5, sizeof(bytes));
//            Check(NetInterface::entropyBytes(nullptr, nullptr, bytes, sizeof(bytes)) == NI_INVALID_STATE,
//                "missing entropy source accepted");
//            for (size_t index = 0; index < sizeof(bytes); ++index)
//            {
//                Check(bytes[index] == 0, "missing entropy source retained output");
//            }
//            Check(NetInterface::entropyBytes(nullptr, nullptr, nullptr, 0) == NI_OK &&
//                NetInterface::entropyBytes(nullptr, nullptr, nullptr, 1) == NI_INVALID_ARGUMENT &&
//                NetInterface::entropyBytes(nullptr, nullptr, bytes, 8193) == NI_INVALID_ARGUMENT,
//                "entropy argument boundaries changed");
//        }
//#ifdef _DEBUG
//        _CrtMemCheckpoint(&after);
//        Check(after.lCounts[_NORMAL_BLOCK] == before.lCounts[_NORMAL_BLOCK] &&
//            after.lSizes[_NORMAL_BLOCK] == before.lSizes[_NORMAL_BLOCK], "failed TLS construction leaked CRT memory");
//#endif
//        std::printf("NetInterface TLS entropy boundary checks=%u PASS\n", checks);
//        return 0;
//    }
//    catch (...)
//    {
//        std::printf("NetInterface TLS entropy boundary checks=%u FAIL\n", checks);
//        return 1;
//    }
//}
// NET_INTERFACE_TLS_ENTROPY_UNIT_END