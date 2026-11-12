#include "LibuvTcpPublicTest.h"
#include <chrono>
#include <vector>
#include <cstdio>
#include <cstring>

LibuvTcpPublicTest::LibuvTcpPublicTest() :
m_instance(nullptr), m_peer(0), m_received(0), m_connected(0), m_closed(0), m_failed(false)
{
}

LibuvTcpPublicTest::~LibuvTcpPublicTest()
{
    LibuvTcp::hostDestroy(m_instance);
}

bool LibuvTcpPublicTest::initialize()
{
    LibuvChannelWritePoolConfig write;
    return LibuvTcp::hostCreate(nullptr, this, &connected, &data, &writable, &closed, &m_instance) == 0 &&
        LibuvTcp::hostConfigureWritePool(m_instance, write) == 0 &&
        LibuvTcp::hostConfigureReadBuffer(m_instance, 16384) == 0;
}

void LibuvTcpPublicTest::connected(void* context, uint64_t peer)
{
    LibuvTcpPublicTest* state = static_cast<LibuvTcpPublicTest*>(context);
    state->m_peer = peer;
    state->m_failed = ++state->m_connected != 1 || peer == 0 || state->m_failed;
}

void LibuvTcpPublicTest::data(void* context, uint64_t peer, const uint8_t* bytes, uint32_t size)
{
    LibuvTcpPublicTest* state = static_cast<LibuvTcpPublicTest*>(context);
    state->m_failed = state->m_failed || peer != state->m_peer;
    for (uint32_t index = 0; index < size; ++index)
    {
        if (bytes[index] != static_cast<uint8_t>(state->m_received + index))
        {
            state->m_failed = true;
        }
    }
    state->m_received += size;
}

void LibuvTcpPublicTest::writable(void* context, uint64_t peer)
{
    LibuvTcpPublicTest* state = static_cast<LibuvTcpPublicTest*>(context);
    state->m_failed = state->m_failed || peer != state->m_peer;
}

void LibuvTcpPublicTest::closed(void* context, uint64_t peer, int32_t reason)
{
    (void)reason;
    LibuvTcpPublicTest* state = static_cast<LibuvTcpPublicTest*>(context);
    state->m_failed = state->m_failed || peer != state->m_peer || ++state->m_closed != 1;
}

int LibuvTcpPublicTest::run(uint32_t payload, uint32_t count, double& milliseconds)
{
    milliseconds = 0;
    if (payload == 0 || payload > 32768 || count == 0 || count > 1000000)
    {
        return 2;
    }
    LibuvTcpPublicTest server;
    LibuvTcpPublicTest client;
    if (!server.initialize() || !client.initialize() ||
        LibuvTcp::hostListen(server.m_instance, "127.0.0.1", 0, 1) != 0)
    {
        return 3;
    }
    uint64_t peer = 0;
    if (LibuvTcp::hostConnect(client.m_instance, "127.0.0.1", LibuvTcp::hostLocalPort(server.m_instance), &peer) != 0)
    {
        return 3;
    }
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(60);
    while ((client.m_connected == 0 || server.m_connected == 0) && std::chrono::steady_clock::now() < deadline)
    {
        if (LibuvTcp::hostPoll(client.m_instance) < 0 || LibuvTcp::hostPoll(server.m_instance) < 0 ||
            client.m_failed || server.m_failed)
        {
            return 4;
        }
    }
    if (client.m_connected != 1 || server.m_connected != 1)
    {
        return 4;
    }
    std::vector<uint8_t> bytes(payload);
    uint32_t submitted = 0;
    const uint64_t expected = static_cast<uint64_t>(payload) * count;
    const auto begin = std::chrono::steady_clock::now();
    while (server.m_received < expected && std::chrono::steady_clock::now() < deadline)
    {
        // 每轮有界发送并驱动双方循环，不能只灌发送队列而阻塞接收。
        for (uint32_t batch = 0; batch < 32 && submitted < count; ++batch)
        {
            const uint64_t offset = static_cast<uint64_t>(submitted) * payload;
            for (uint32_t index = 0; index < payload; ++index)
            {
                bytes[index] = static_cast<uint8_t>(offset + index);
            }
            const int32_t result = LibuvTcp::hostWrite(client.m_instance, peer, &bytes[0], payload);
            if (result == 1)
            {
                break;
            }
            if (result != 0)
            {
                return 4;
            }
            ++submitted;
        }
        if (LibuvTcp::hostPoll(client.m_instance) < 0 || LibuvTcp::hostPoll(server.m_instance) < 0 ||
            client.m_failed || server.m_failed || client.m_closed != 0 || server.m_closed != 0)
        {
            return 4;
        }
    }
    milliseconds = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - begin).count();
    if (submitted != count || server.m_received != expected)
    {
        return 4;
    }
    const bool destroyed = LibuvTcp::hostDestroy(client.m_instance) == 0;
    if (!destroyed)
    {
        return 5;
    }
    client.m_instance = nullptr;
    if (LibuvTcp::hostDestroy(server.m_instance) != 0)
    {
        return 5;
    }
    server.m_instance = nullptr;
    return !client.m_failed && !server.m_failed && client.m_closed == 1 && server.m_closed == 1 ? 0 : 5;
}

#ifdef LIBUV_TEST_PUBLIC_DLL
#include <thread>

LibuvTcpDllTask::LibuvTcpDllTask(LibuvTcp& tcp) :
m_tcp(tcp)
{

}

void LibuvTcpDllTask::doTask()
{
    m_tcp.loop();
}

void LibuvTcpDllTask::stopTask()
{
    m_tcp.stop();
}

LibuvTcpDllSender::LibuvTcpDllSender() :
m_target(nullptr), m_failed(false), m_received(0)
{

}

void LibuvTcpDllSender::uvServerConnected(uv_tcp_t* target)
{
    m_target.store(target, std::memory_order_release);
}

void LibuvTcpDllSender::uvServerDisconnected(uv_tcp_t* target)
{
    (void)target;
    m_target.store(nullptr, std::memory_order_release);
}

void LibuvTcpDllSender::uvServerNotFind()
{
    m_failed.store(true);
}

void LibuvTcpDllSender::data(void* context, uint64_t peer, const uint8_t* bytes, uint32_t size)
{
    (void)peer;
    LibuvTcpDllSender* self = static_cast<LibuvTcpDllSender*>(context);
    uint8_t expected[10];
    const uint32_t length = 6;
    std::memcpy(expected, &length, 4);
    std::memcpy(expected + 4, "123456", 6);
    for (uint32_t index = 0; index < size; ++index)
    {
        if (bytes[index] != expected[(self->m_received + index) % 10])
        {
            self->m_failed.store(true);
        }
    }
    self->m_received += size;
}

void LibuvTcpDllSender::notify(void* context, uint64_t peer)
{
    (void)context;
    (void)peer;
}

void LibuvTcpDllSender::closed(void* context, uint64_t peer, int32_t reason)
{
    (void)context;
    (void)peer;
    (void)reason;
}

bool LibuvTcpDllSender::run()
{
    LibuvTcpDllSender client;
    void* server = nullptr;
    if (LibuvTcp::hostCreate(nullptr, &client, &notify, &data, &notify, &closed, &server) != 0)
    {
        return false;
    }
    uint32_t loop = 0;
    bool passed = false;
    try
    {
        if (LibuvTcp::hostListen(server, "127.0.0.1", 0, 1) == 0 &&
            client.initClient("127.0.0.1", static_cast<int32_t>(LibuvTcp::hostLocalPort(server))))
        {
            // 经统一线程管理器驱动真实DLL循环，不访问LibuvTcp私有布局或测试钩子。
            loop = CTaskThreadManager::instance().init();
            const std::shared_ptr<CTaskThread> worker = CTaskThreadManager::instance().getThreadInterface(loop);
            const std::shared_ptr<CTask> task(new LibuvTcpDllTask(client));
            if (loop == 0 || !worker || !worker->tryPostTaskResult(task, 1, true, true).m_committed)
            {
                client.m_failed.store(true);
            }
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(15);
            uint32_t submitted = 0;
            while (client.m_received < 100000 && !client.m_failed.load() &&
                std::chrono::steady_clock::now() < deadline)
            {
                uv_tcp_t* target = client.m_target.load(std::memory_order_acquire);
                if (target != nullptr && submitted < 10000)
                {
                    const uint32_t mode = submitted % 4;
                    const bool accepted = mode == 0 ? client.send(target, "123456", 6) :
                        mode == 1 ? client.post(target, "123456", 6) :
                        mode == 2 ? client.trySend(target, "123456", 6, true, true) == 0 :
                        client.tryPost(target, "123456", 6, true, true) == 0;
                    if (!accepted)
                    {
                        client.m_failed.store(true);
                    }
                    ++submitted;
                }
                if (LibuvTcp::hostPoll(server) < 0)
                {
                    client.m_failed.store(true);
                }
                if (target == nullptr || submitted == 10000)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }
            passed = submitted == 10000 && client.m_received == 100000 && !client.m_failed.load();
        }
    }
    catch (...)
    {
        passed = false;
    }
    client.stop();
    if (loop != 0)
    {
        passed = CTaskThreadManager::instance().uninit(loop).m_status == CTaskThreadOk && passed;
    }
    return LibuvTcp::hostDestroy(server) == 0 && passed;
}

/** 独立 DLL 消费者，仅链接本库导入库，不包含任何私有实现
@return 完整字节及终态验证结果
*/
int main()
{
    try
    {
        // 分配和失败后的释放都调用DLL导出接口，不由消费者的静态CRT释放库内数组。
        LibuvTcp allocation;
        LibuvQueuePoolConfig queue;
        LibuvConnectionPoolConfig connections;
        LibuvSendPoolConfig send;
        LibuvReadPoolConfig read;
        if (allocation.configureSendQueue(queue) != 0 || allocation.configureCloseQueue(queue) != 0 ||
            allocation.configureConnectionPool(connections) != 0 || allocation.configureSendPool(send) != 0 ||
            allocation.configureReadPool(read) != 0)
        {
            return 11;
        }
        const int32_t size = static_cast<int32_t>(sizeof(void*) + 4);
        char* bytes = allocation.memoryForSend(size);
        std::memset(bytes, 0, size);
        const int32_t submitted = allocation.trySend(bytes);
        if (submitted != 0)
        {
            allocation.releaseSendMemory(bytes);
        }
        allocation.releaseSendMemory(nullptr);
        char* posted = allocation.memoryForSend(size);
        std::memset(posted, 0, size);
        const int32_t postResult = allocation.tryPost(posted);
        if (postResult != 0)
        {
            allocation.releaseSendMemory(posted);
        }
        char* owned = allocation.memoryForSend(size);
        std::memset(owned, 0, size);
        if (allocation.post(owned) || postResult >= 0)
        {
            return 9;
        }
        LibuvSendBuffer body = { "123456", 6 };
        if (allocation.trySendBatch(nullptr, &body, 1) >= 0 || allocation.sendBatch(nullptr, &body, 1) ||
            allocation.trySend(nullptr, "123456", 6) >= 0 || allocation.send(nullptr, "123456", 6) ||
            allocation.tryPostBatch(nullptr, &body, 1) >= 0 || allocation.postBatch(nullptr, &body, 1) ||
            allocation.tryPost(nullptr, "123456", 6) >= 0 || allocation.post(nullptr, "123456", 6) ||
            allocation.initServer(static_cast<const char*>(nullptr), 0))
        {
            return 8;
        }
        if (submitted >= 0)
        {
            return 7;
        }
        double milliseconds = 0;
        const int result = LibuvTcpPublicTest::run(1024, 10000, milliseconds);
        std::printf("public DLL transfer: result=%d ms=%.3f\n", result, milliseconds);
        const bool sender = LibuvTcpDllSender::run();
        std::printf("public DLL send/post: %s frames=10000\n", sender ? "PASS" : "FAIL");
        return result == 0 && sender ? 0 : 10;
    }
    catch (...)
    {
        return 6;
    }
}
#endif