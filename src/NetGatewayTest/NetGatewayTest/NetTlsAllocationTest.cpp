#include "NetTlsAllocationTest.h"
#include "NetIsolatedTest.h"
#include "NetInterface/NetInterfaceAPI.h"
#include <cstdio>
#include <new>
#include <stdexcept>

bool NetTlsAllocationTest::m_refusing = false;

/** 测试程序的nothrow标量分配；未启用故障时沿用标准分配与new_handler语义
@param [in] size 需要分配的字节数
@param [in] tag 标准nothrow调用标签
@return 内存或空指针，不向调用者传播分配异常
*/
void* operator new(std::size_t size, const std::nothrow_t& tag) throw()
{
    (void)tag;
    if (NetTlsAllocationTest::refusing())
    {
        return nullptr;
    }
    try
    {
        return ::operator new(size);
    }
    catch (...)
    {
        return nullptr;
    }
}

bool NetTlsAllocationTest::refusing()
{
    return m_refusing;
}

int NetTlsAllocationTest::check()
{
    bool passed = false;
    m_refusing = true;
    try
    {
        NetInterfaceTlsOptions options = {};
        NetInterfaceTls tls(options);
        const uint8_t* output = reinterpret_cast<const uint8_t*>(1);
        uint32_t size = 7;
        uint32_t accepted = 7;
        uint32_t received = 7;
        uint32_t consumed = 7;
        passed = tls.status() == NET_INTERFACE_TLS_MEMORY && tls.detail() == 0 &&
            tls.feed(nullptr, 0, consumed) == NET_INTERFACE_TLS_MEMORY && consumed == 0 &&
            tls.progress(1) == NET_INTERFACE_TLS_MEMORY &&
            tls.write(nullptr, 0, accepted) == NET_INTERFACE_TLS_MEMORY && accepted == 0 &&
            tls.read(nullptr, 0, received) == NET_INTERFACE_TLS_MEMORY && received == 0 &&
            tls.peekOutput(output, size) == NET_INTERFACE_TLS_MEMORY && output == nullptr && size == 0 &&
            tls.consumeOutput(0) == NET_INTERFACE_TLS_MEMORY &&
            tls.finishInput() == NET_INTERFACE_TLS_MEMORY && tls.close() == NET_INTERFACE_TLS_MEMORY &&
            tls.outputState() == NET_INTERFACE_TLS_MEMORY;
    }
    catch (...)
    {
        passed = false;
    }
    m_refusing = false;
    std::printf("TLS_ALLOCATION_FAILURE %s\n", passed ? "PASS" : "FAIL");
    return passed ? 0 : 1;
}

void NetTlsAllocationTest::run()
{
    if (NetIsolatedTest::run("--isolated-manager=tls-allocation") != 0)
    {
        throw std::runtime_error("TLS null allocation contract failed");
    }
}