#include "NetDirectoryCacheTest.h"
#include "NetIsolatedTest.h"
#include "NetPeerTestAccess.h"
#include <stdexcept>
#include <cstdio>
#include <cstring>
/** 测试专用确定性熵，不连接网络或签发身份。
@param [in] context 未使用
@param [out] output 临时名输入
@param [in] size 字节数
@return 0
*/
static int32_t DirectoryControlledEntropy(void* context, uint8_t* output, uint32_t size)
{
    (void)context;
    std::memset(output, 0x55, size);
    return 0;
}

/** 忙不得丢工作，畸形元数据不得产生可信缓存，停止必须等done。
@return 0通过，1失败
*/
int NetDirectoryCacheTest::check()
{
    uint32_t assertions = 0;
    uint32_t scenario = 0;
#define CONTROLLED_CHECK(condition) { ++assertions; if (!(condition)) { std::printf("FAIL scenario=%u line=%u: %s\n", scenario, static_cast<uint32_t>(__LINE__), #condition); return 1; } }
    CONTROLLED_CHECK(NetFileIo::configureBackend(NetDirectoryStoreTestBackend::create) == NI_OK);
    for (uint32_t test = 0; test < 13; ++test)
    {
        scenario = test;
        NetGatewayDirectoryStore store;
        CONTROLLED_CHECK(store.open("controlled.nodes.cache", DirectoryControlledEntropy) == NI_OK);
        NetDirectoryStoreTestBackend* backend = static_cast<NetDirectoryStoreTestBackend*>(NetFileIoTestBackend::m_current);
        backend->m_busy = 2;
        CONTROLLED_CHECK(store.poll() == NI_OK && store.poll() == NI_OK && backend->m_accepted == 0);
        CONTROLLED_CHECK(store.poll() == NI_OK && backend->m_accepted == 1 && backend->m_job.operation == NET_FILE_IO_OPEN_READ);
        NetInterfaceDirectoryNodeData loaded[64] = {};
        for (auto& record : loaded)
        {
            record.m_structSize = sizeof(record);
            record.m_version = 1;
        }
        uint32_t loadedCount = 0;
        const uint8_t invalid[48] = {};
        if (test < 2)
        {
            backend->finish(NI_OK, 41, 48);
            if (test == 0)
            {
                ++backend->m_reply.id;
            }
            else
            {
                backend->m_reply.operation = NET_FILE_IO_WRITE;
            }
            CONTROLLED_CHECK(store.poll() == NI_PROTOCOL_ERROR && backend->m_stopped && !store.stopped());
            CONTROLLED_CHECK(backend->m_consumed == 0 && store.takeLoaded(loaded, 64, loadedCount) == NI_NEED_MORE);
        }
        else if (test <= 5 || test == 9)
        {
            backend->finish(NI_OK, test == 2 ? 0 : 41, test == 3 ? NET_INTERFACE_DIRECTORY_BYTES + 49 : 48);
            store.poll();
            if (test == 4 || test == 5 || test == 9)
            {
                CONTROLLED_CHECK(backend->m_job.operation == NET_FILE_IO_READ);
                backend->finish(NI_OK, 41, 48, test == 5 ? nullptr : invalid, test == 4 ? 47 : 48);
                CONTROLLED_CHECK(store.poll() < 0);
            }
            CONTROLLED_CHECK(store.takeLoaded(loaded, 64, loadedCount) == NI_NEED_MORE && loadedCount == 0 && loaded[0].m_name[0] == 0);
            if (test == 5)
            {
                // 非空长度配空指针在文件门面即被拒绝，未知完成不能继续发送CLOSE。
                CONTROLLED_CHECK(backend->m_stopped && !store.stopped() && backend->m_consumed == 1);
                CONTROLLED_CHECK(backend->m_job.operation == NET_FILE_IO_READ);
            }
            else if (test != 2)
            {
                CONTROLLED_CHECK(backend->m_job.operation == NET_FILE_IO_CLOSE && backend->m_job.file == 41);
                backend->finish(NI_OK, 0, 0);
                CONTROLLED_CHECK(store.poll() < 0);
            }
        }
        else if (test == 12)
        {
            // 目录由NI持有，测试侧只提供字节和固定值，兼容独立DLL堆
            NetInterfaceDirectoryNodeData node = {};
            node.m_structSize = sizeof(node);
            node.m_version = 1;
            std::memcpy(node.m_name, "node.0123456789abcdef0123456789abcdef", sizeof(node.m_name));
            std::memcpy(node.m_address, "127.0.0.1", 10);
            node.m_port = 23456;
            node.m_capacity = 32;
            node.m_boot = UINT64_C(0xfedcba9876543210);
            NetInterfaceDirectoryHandle directory = nullptr;
            CONTROLLED_CHECK(NetInterface::directoryCreateNodes(&node, 1, &directory) == NI_OK);
            uint32_t bytesSize = 0;
            const uint64_t now = NetGatewayDirectoryStore::utc();
            CONTROLLED_CHECK(NetInterface::directoryCache(directory, static_cast<uint32_t>(now),
                static_cast<uint32_t>(now >> 32), nullptr, 0, &bytesSize) == NI_BUFFER_TOO_SMALL);
            std::vector<uint8_t> bytes(bytesSize);
            CONTROLLED_CHECK(NetInterface::directoryCache(directory, static_cast<uint32_t>(now),
                static_cast<uint32_t>(now >> 32), bytes.data(), bytesSize, &bytesSize) == NI_OK);
            NetInterface::directoryDestroy(directory);
            backend->finish(NI_OK, 41, bytesSize);
            CONTROLLED_CHECK(store.poll() == NI_OK && backend->m_job.operation == NET_FILE_IO_READ);
            backend->finish(NI_OK, 41, bytesSize, bytes.data(), bytesSize);
            CONTROLLED_CHECK(store.poll() == NI_OK && backend->m_job.operation == NET_FILE_IO_CLOSE);
            CONTROLLED_CHECK(store.takeLoaded(nullptr, 0, loadedCount) == NI_BUFFER_TOO_SMALL && loadedCount == 1);
            const NetInterfaceDirectoryNodeData before = loaded[0];
            CONTROLLED_CHECK(store.takeLoaded(loaded, 0, loadedCount) == NI_BUFFER_TOO_SMALL && loadedCount == 1 &&
                std::memcmp(loaded, &before, sizeof(before)) == 0);
            loaded[0].m_version = 2;
            CONTROLLED_CHECK(store.takeLoaded(loaded, 64, loadedCount) == NI_INVALID_ARGUMENT && loadedCount == 0 &&
                loaded[0].m_version == 2 && loaded[0].m_name[0] == 0);
            loaded[0].m_version = 1;
            CONTROLLED_CHECK(store.takeLoaded(loaded, 64, loadedCount) == NI_READY && loadedCount == 1 &&
                loaded[0].m_port == node.m_port && loaded[0].m_boot == node.m_boot &&
                std::strcmp(loaded[0].m_name, node.m_name) == 0 && std::strcmp(loaded[0].m_address, node.m_address) == 0);
            const NetInterfaceDirectoryNodeData consumed = loaded[0];
            CONTROLLED_CHECK(store.takeLoaded(loaded, 64, loadedCount) == NI_NEED_MORE && loadedCount == 0 &&
                std::memcmp(loaded, &consumed, sizeof(consumed)) == 0);
            backend->finish(NI_OK, 0, 0);
            CONTROLLED_CHECK(store.poll() == NI_OK);
        }
        else if (test == 10)
        {
            store.shutdown();
            CONTROLLED_CHECK(backend->m_stopped && !store.stopped());
            store.poll();
            CONTROLLED_CHECK(!store.stopped());
        }
        else
        {
            backend->finish(NI_NOT_FOUND, 0, 0);
            store.poll();
            NetInterfaceDirectoryPoint point;
            point.m_name = "node.0123456789abcdef0123456789abcdef";
            point.m_address = "127.0.0.1";
            point.m_port = 23456;
            point.m_capacity = 32;
            point.m_boot = 9;
            std::vector<NetInterfaceDirectoryPoint> points(1, point);
            CONTROLLED_CHECK(store.queue(points) == NI_OK);
            store.poll();
            CONTROLLED_CHECK(backend->m_job.operation == NET_FILE_IO_OPEN_TEMPORARY);
            backend->finish(NI_OK, test == 11 ? 0 : 41, 0);
            backend->m_busy = test == 6 ? 2 : 0;
            store.poll();
            if (test == 6)
            {
                CONTROLLED_CHECK(backend->m_job.id == 0 && backend->m_copy.empty());
                store.poll();
                CONTROLLED_CHECK(backend->m_job.id == 0);
                store.poll();
            }
            if (test == 11)
            {
                CONTROLLED_CHECK(backend->m_job.id == 0 && store.poll() == NI_PROTOCOL_ERROR);
            }
            else
            {
                CONTROLLED_CHECK(backend->m_job.operation == NET_FILE_IO_WRITE && backend->m_job.flags == 1 &&
                    backend->m_copy.size() > 48 && std::memcmp(backend->m_copy.data(), "NGC3", 4) == 0);
                backend->finish(NI_OK, 41, backend->m_copy.size() - (test == 7 ? 1 : 0));
                store.poll();
                CONTROLLED_CHECK(backend->m_job.operation == static_cast<uint32_t>(test == 7 ? NET_FILE_IO_CLOSE : NET_FILE_IO_PUBLISH));
                backend->finish(test == 8 ? NI_INVALID_STATE : NI_OK, 0, 0);
                const int32_t result = store.poll();
                CONTROLLED_CHECK(test == 6 ? result == NI_OK : result < 0);
            }
        }
        store.shutdown();
        CONTROLLED_CHECK(!store.stopped());
        const uint32_t destroyed = NetFileIoTestBackend::m_destroyed;
        backend->m_done = true;
        store.poll();
        CONTROLLED_CHECK(store.stopped() && NetFileIoTestBackend::m_destroyed == destroyed + 1);
    }
    std::printf("Directory cache controlled assertions=%u PASS\n", assertions);
    return 0;
#undef CONTROLLED_CHECK
}

void NetDirectoryCacheTest::run()
{
    if (NetIsolatedTest::run("--isolated-file-case=cache") != 0)
    {
        throw std::runtime_error("NetDirectoryCacheTest failed");
    }
}