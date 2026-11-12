#include "ProcessWorkCoreTests.h"
#include "ProcessWorkTest.h"
#include "ProcessWork/ProcessWorkLocalQueue.h"
#include <cstring>
#include <limits>
#include <thread>

void testProcessWorkLocalBudget()
{
    ProcessWorkLocalQueue queue;
    ProcessWorkMemoryConfig config;
    config.m_localQueueBytes = 300;
    bbCheck(queue.configure(config), "local queue configure failed");
    LockFreeQueueResult result;
    char* packet = queue.allocate(32, result);
    bbCheck(packet != nullptr && result == LockFreeQueueSuccess && queue.used() == 296, "wrong allocation charge");
    bbCheck(queue.allocate(32, result) == nullptr && result == LockFreeQueueCapacityLimit, "local budget bypassed");
    bbCheck(!queue.configure(config), "live budget reconfigured");
    bbCheck(queue.submit(nullptr) == LockFreeQueueInvalidConfiguration && !queue.take(nullptr, 0), "invalid queue input accepted");
    bbCheck(queue.submit(packet) == LockFreeQueueSuccess, "local publish failed");
    char* popped = nullptr;
    bbCheck(queue.take(&popped, 0) && popped == packet && queue.used() == 296, "pop prematurely returned budget");
    queue.release(popped);
    bbCheck(queue.used() == 0, "finished packet retained budget");
    packet = queue.allocate(1024, result);
    bbCheck(packet == nullptr && result == LockFreeQueueInvalidConfiguration, "fixed quota admitted an oversized singleton");
    queue.release(packet);
    bbCheck(queue.allocate((std::numeric_limits<size_t>::max)(), result) == nullptr && queue.used() == 0, "overflow request reserved capacity");
    packet = queue.allocate(32, result);
    bbCheck(packet != nullptr && queue.submit(packet) == LockFreeQueueSuccess, "cleanup setup failed");
    queue.clear();
    bbCheck(queue.used() == 0 && queue.empty(), "clear leaked packet charge");
    config.m_reserveBytes = (std::numeric_limits<uint64_t>::max)();
    bbCheck(queue.configure(config) && queue.allocate(1, result) == nullptr && queue.used() == 0, "memory pressure admitted allocation");
}

/** 对默认分配与预留小包分别验证并发归还、预算与每线程FIFO
@param [in] reserve 预留小包槽数，0使用原分配路径
*/
static void LocalQueueStress(uint32_t reserve)
{
    ProcessWorkLocalQueue queue;
    ProcessWorkMemoryConfig config;
    config.m_localQueueBytes = 1024;
    config.m_postBufferReserve = reserve;
    bbCheck(queue.configure(config), "stress configure failed");
    std::atomic<bool> cancel(false);
    IpcJobs jobs;
    const int32_t messages = 5000;
    for (int32_t producer = 0; producer < 8; ++producer)
    {
        jobs.add([&queue, &cancel, producer, messages]()
        {
            for (int32_t sequence = 0; sequence < messages && !cancel.load(); ++sequence)
            {
                LockFreeQueueResult result;
                char* packet = nullptr;
                while (!cancel.load() && (packet = queue.allocate(8, result)) == nullptr)
                {
                    std::this_thread::yield();
                }
                if (packet == nullptr)
                {
                    return;
                }
                const int32_t body[2] = { producer, sequence };
                std::memcpy(packet, body, sizeof(body));
                result = LockFreeQueueTemporaryUnavailable;
                while (!cancel.load() && (result = queue.submit(packet)) != LockFreeQueueSuccess)
                {
                    std::this_thread::yield();
                }
                if (result != LockFreeQueueSuccess)
                {
                    queue.release(packet);
                }
            }
        });
    }
    int32_t expected[8] = {};
    const uint64_t deadline = bbNow() + 30000;
    try
    {
        for (int32_t received = 0; received < messages * 8;)
        {
            char* packet = nullptr;
            bbCheck(bbNow() < deadline, "local queue stress deadline");
            if (!queue.take(&packet, 10))
            {
                continue;
            }
            int32_t body[2] = {};
            std::memcpy(body, packet, sizeof(body));
            queue.release(packet);
            bbCheck(body[0] >= 0 && body[0] < 8 && body[1] == expected[body[0]], "local queue ordering/duplication failure");
            ++expected[body[0]];
            ++received;
        }
        jobs.drain();
        bbCheck(queue.used() == 0 && queue.empty(), "local queue stress retained charges");
    }
    catch (...)
    {
        cancel.store(true);
        throw;
    }
}

void testProcessWorkLocalQueueStress()
{
    LocalQueueStress(0);
}

void testProcessWorkPostPacketPool()
{
    ProcessWorkLocalQueue queue;
    ProcessWorkMemoryConfig config;
    config.m_postBufferReserve = 7;
    config.m_localQueueBytes = 4096;
    bbCheck(queue.configure(config), "post packet reserve failed");
    // 不足一个热窗口的小池，填满后仍允许合法请求从原分配器取得正文。
    for (int32_t round = 0; round < 40; ++round)
    {
        std::vector<char*> packets;
        for (int32_t i = 0; i < 9; ++i)
        {
            LockFreeQueueResult status;
            const size_t bytes = i == 8 ? 53 : 52;
            char* packet = queue.allocate(bytes, status, true);
            bbCheck(packet != nullptr && status == LockFreeQueueSuccess, "packet pool fallback rejected valid request");
            std::memset(packet, i, bytes);
            packets.push_back(packet);
        }
        bbCheck(!queue.configure(config), "packet pool replaced while borrowed");
        for (int32_t i = 8; i >= 0; --i)
        {
            const size_t bytes = i == 8 ? 53 : 52;
            bbCheck(packets[i][0] == i && packets[i][bytes - 1] == i, "packet pool reused an owned slot");
        }
        queue.release(packets[4]);
        packets[4] = nullptr;
        queue.releasePrefix(packets.data(), packets.size());
        queue.releasePrefix(packets.data(), packets.size());
        queue.releasePrefix(nullptr, 9);
        for (size_t i = 0; i < packets.size(); ++i)
        {
            bbCheck(packets[i] == nullptr, "prefix release did not clear ownership");
        }
        bbCheck(queue.used() == 0, "packet pool release leaked accounting");
    }
    for (int32_t i = 0; i < 9; ++i)
    {
        LockFreeQueueResult status;
        char* packet = queue.allocate(sizeof(i), status, true);
        bbCheck(packet != nullptr, "prefix fixture allocation failed");
        std::memcpy(packet, &i, sizeof(i));
        bbCheck(queue.submit(packet) == LockFreeQueueSuccess, "prefix fixture admission failed");
    }
    char* prefix[4] = {};
    size_t consumed = 99;
    bbCheck(!queue.takePrefix(nullptr, 4, consumed, 0) && consumed == 0, "invalid prefix consumed ownership");
    bbCheck(queue.takePrefix(prefix, 4, consumed, 0) && consumed >= 1 && consumed <= 4,
        "prefix did not transfer a valid ready prefix");
    for (int32_t i = 0; i < 9; ++i)
    {
        char* packet = i < static_cast<int32_t>(consumed) ? prefix[i] : nullptr;
        bbCheck(packet != nullptr || queue.pop(&packet), "prefix/single mix lost a packet");
        int32_t value = -1;
        std::memcpy(&value, packet, sizeof(value));
        queue.release(packet);
        bbCheck(value == i, "prefix/single mix changed FIFO");
    }
    bbCheck(queue.used() == 0 && queue.empty(), "prefix release retained charges");
    config.m_localQueueBytes = 100;
    bbCheck(queue.configure(config), "idle packet reserve must not be counted as message backlog");
    LockFreeQueueResult oversized;
    bbCheck(queue.allocate(1, oversized) == nullptr && oversized == LockFreeQueueInvalidConfiguration,
        "prepared idle cache bypassed fixed message quota");
    config.m_localQueueBytes = 4096;
    config.m_reserveBytes = UINT64_MAX;
    bbCheck(!queue.configure(config), "queue preparation ignored system reserve");
    LocalQueueStress(16);
}

void testProcessWorkPostBudget()
{
    ProcessWork& ipc = ProcessWork::instance();
    PacketLog log;
    IpcScope scope;
    ProcessWorkMemoryConfig memory;
    memory.m_localQueueBytes = 300;
    bbCheck(ipc.configurePostMemory(memory), "post memory configuration failed");
    const std::string body(16, 'x');
    bbCheck(ipc.tryPost(bbPid(), body.data(), 16, static_cast<MessageType>(1)) == PROCESS_WORK_POSTED, "first tryPost failed");
    bbCheck(ipc.postPendingBytes() == 296, "post charge incorrect");
    bbCheck(ipc.tryPost(bbPid(), body.data(), 16, static_cast<MessageType>(2)) == PROCESS_WORK_POST_CAPACITY, "over-budget tryPost must report capacity, not competition");
    bbCheck(ipc.tryPost(0, body.data(), 16) == PROCESS_WORK_POST_INVALID &&
        ipc.tryPost(std::string(), body.data(), 16) == PROCESS_WORK_POST_INVALID, "invalid tryPost admitted");
    bbCheck(!ipc.configurePostMemory(memory) && ipc.postPendingBytes() == 296, "failed post/configuration changed charge");
    bbCheck(ipc.tryPost(bbPid(), body.data(), 16, static_cast<MessageType>(3), true, false) == PROCESS_WORK_POST_CAPACITY,
        "contention retry must preserve capacity result");
    bbCheck(ipc.postResult(bbPid(), body.data(), 16, static_cast<MessageType>(3)) == PROCESS_WORK_POST_UNAVAILABLE &&
        !ipc.post(bbPid(), body, static_cast<MessageType>(3)), "post cannot wait for an unstarted consumer");
    bbCheck(ipc.postPendingBytes() == 296, "failed compatibility post changed pending bytes");
    scope.start(&log, receiveName("post-budget"), 64, 2);
    ipc.initPostThread();
    ipc.waitForPostEnd();
    waitPackets(log, 1);
    bbCheck(log.snapshot()[0].type == 1 && log.snapshot()[0].body == body && ipc.postPendingBytes() == 0, "failed post delivered or completed charge retained");
    ipc.uninitPostThread();
    bbCheck(ipc.configurePostMemory(ProcessWorkMemoryConfig()), "post configuration not reusable after drain");
}

void testProcessWorkResultContracts()
{
    ProcessWork& ipc = ProcessWork::instance();
    PacketLog log;
    IpcScope scope;
    ProcessWorkMemoryConfig memory;
    memory.m_reserveBytes = (std::numeric_limits<uint64_t>::max)();
    bbCheck(ipc.configurePostMemory(memory), "resource guard configuration rejected");
    bbCheck(ipc.tryPost(bbPid(), "123456", 6) == PROCESS_WORK_POST_RESOURCE &&
        ipc.postResult(bbPid(), "123456", 6) == PROCESS_WORK_POST_RESOURCE && ipc.postPendingBytes() == 0,
        "memory reserve pressure reported as competition or consumed budget");
    bbCheck(ipc.configurePostMemory(ProcessWorkMemoryConfig()), "default post budget restore failed");
    const std::string name = receiveName("results");
    scope.start(&log, name, 18, 4);
    bbCheck(!ipc.addProcessReceiveCallback(&log) && !ipc.setReceiveName("changed-while-running"),
        "running receive state accepted lifecycle mutation");
    bbCheck(ipc.sendResult(0, "123456", 6) == PROCESS_WORK_INVALID &&
        ipc.sendResult(name, nullptr, 1) == PROCESS_WORK_INVALID &&
        ipc.sendResult(std::string(), "123456", 6) == PROCESS_WORK_INVALID &&
        ipc.sendResult(bbPid(), "1234567", 7) == PROCESS_WORK_TOO_LARGE,
        "invalid result send was admitted");
    bbCheck(ipc.sendResult(bbPid(), "123456", 6, static_cast<MessageType>(1)) == PROCESS_WORK_SENT &&
        ipc.sendResult(name, nullptr, 0, static_cast<MessageType>(2)) == PROCESS_WORK_SENT,
        "valid result send failed");
    bbCheck(ipc.postResult(0, "123456", 6) == PROCESS_WORK_POST_INVALID &&
        ipc.postResult(std::string(), "123456", 6) == PROCESS_WORK_POST_INVALID,
        "invalid result post was admitted");
    bbCheck(ipc.postResult(bbPid(), "123456", 6, static_cast<MessageType>(3)) == PROCESS_WORK_POSTED &&
        ipc.postResult(name, "123456", 6, static_cast<MessageType>(4)) == PROCESS_WORK_POSTED,
        "valid result post failed");
    waitPackets(log, 2);
    bbCheck(ipc.initPostThread(), "post startup did not report success");
    const uint32_t activeThreads = CTaskThreadManager::instance().count();
    bbCheck(ipc.initPostThread() && CTaskThreadManager::instance().count() == activeThreads,
        "repeated post initialization changed resources or reported failure");
    ipc.waitForPostEnd();
    waitPackets(log, 4);
    const std::vector<ReceivedPacket> received = log.snapshot();
    bbCheck(received[0].type == 1 && received[1].type == 2 && received[2].type == 3 && received[3].type == 4 &&
        received[0].body == "123456" && received[1].body.empty() && received[2].body == "123456" && received[3].body == "123456",
        "result interface lost, duplicated or reordered messages");
}

void testProcessWorkPostContention()
{
    ProcessWork& ipc = ProcessWork::instance();
    PacketLog log;
    IpcScope scope;
    scope.start(&log, receiveName("post-contention"), 64, 128);
    bbCheck(ipc.configurePostMemory(ProcessWorkMemoryConfig()), "post contention budget setup failed");
    std::atomic<int32_t> ready(0);
    std::atomic<bool> go(false);
    IpcJobs jobs;
    const int32_t count = 5000;
    try
    {
        for (int32_t thread = 0; thread < 32; ++thread)
        {
            // 每条正文只有123456；序号使用既有type字段，不往正文追加测试元数据。
            jobs.add([&ipc, &ready, &go, thread]()
            {
                ++ready;
                while (!go.load(std::memory_order_acquire))
                {
                    std::this_thread::yield();
                }
                for (int32_t sequence = 0; sequence < 5000; ++sequence)
                {
                    ipc.post(bbPid(), "123456", 6, static_cast<MessageType>(thread * 5000 + sequence));
                }
            });
        }
        bbWait([&ready]() { return ready.load() == 32; }, "ordinary post producer barrier");
        go.store(true, std::memory_order_release);
        jobs.drain();
    }
    catch (...)
    {
        go.store(true, std::memory_order_release);
        throw;
    }
    bbCheck(ipc.postPendingBytes() == 0 && !ipc.configurePostMemory(ProcessWorkMemoryConfig()),
        "unlimited post counted bytes or lost in-flight lifetime protection");
    ipc.initPostThread();
    ipc.waitForPostEnd();
    waitPackets(log, 32 * count);
    const std::vector<ReceivedPacket> packets = log.snapshot();
    int32_t expected[32] = {};
    for (size_t i = 0; i < packets.size(); ++i)
    {
        const int32_t thread = packets[i].type / count;
        bbCheck(thread >= 0 && thread < 32 && packets[i].type % count == expected[thread] &&
            packets[i].pid == bbPid() && packets[i].body == "123456", "ordinary post reordered or corrupted fixed payload");
        ++expected[thread];
    }
    bbCheck(log.errors == 0 && packets.size() == 32 * count && ipc.postPendingBytes() == 0, "ordinary post incomplete");
}

void testProcessWorkSlowCallback()
{
    ProcessWork& ipc = ProcessWork::instance();
    PacketLog log;
    IpcScope scope;
    std::atomic<bool> entered(false), resume(false);
    log.action = [&entered, &resume](const ReceivedPacket& packet)
    {
        if (packet.type == 0)
        {
            entered.store(true);
            bbWait([&resume]() { return resume.load(); }, "release deliberately slow callback", 10000);
        }
    };
    ProcessWorkMemoryConfig memory;
    memory.m_localQueueBytes = 300;
    ipc.addProcessReceiveCallback(&log);
    bbCheck(ipc.setReceiveName(receiveName("slow-callback")) && ipc.initReceive(28, 2, 0, 0, 0, 0, memory), "slow callback setup failed");
    const std::string body(16, 'x');
    try
    {
        ipc.send(bbPid(), body, static_cast<MessageType>(0));
        bbWait([&entered]() { return entered.load(); }, "slow callback entered");
        bbCheck(ipc.receivePendingBytes() == 293, "executing callback not charged");
        for (int32_t type = 1; type <= 2; ++type)
        {
            bbWait([&ipc, &body, type]()
            {
                const int32_t result = ipc.trySend(bbPid(), body.data(), 16, static_cast<MessageType>(type));
                bbCheck(result == PROCESS_WORK_SENT || result == PROCESS_WORK_BUSY || result == PROCESS_WORK_CAPACITY || result == PROCESS_WORK_RESOURCE, "shared admission failed");
                return result == PROCESS_WORK_SENT;
            }, "shared slot returned by reader");
        }
        bbCheck(ipc.trySend(bbPid(), body.data(), 16, static_cast<MessageType>(3)) == PROCESS_WORK_CAPACITY, "slow callback bypassed shared pressure");
        bbSleep(100);
        bbCheck(ipc.receivePendingBytes() == 293 && ipc.receiveBufferCount() == 2, "slow callback caused uncontrolled local allocation");
        resume.store(true);
        waitPackets(log, 3);
        bbWait([&ipc]() { return ipc.receivePendingBytes() == 0; }, "callback charges returned");
        const std::vector<ReceivedPacket> packets = log.snapshot();
        for (int32_t i = 0; i < 3; ++i)
        {
            bbCheck(packets[i].type == i && packets[i].body == body, "pressure retry changed payload/order");
        }
    }
    catch (...)
    {
        resume.store(true);
        throw;
    }
}