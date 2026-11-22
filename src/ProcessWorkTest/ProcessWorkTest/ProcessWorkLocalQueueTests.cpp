#include "ProcessWorkCoreTests.h"
#include "ProcessWorkMemoryTests.h"
#include "ProcessWorkTest.h"
#include "ProcessWork/ProcessWorkLocalQueue.h"
#include <algorithm>
#include <cstring>
#ifdef _WIN32
#include <Windows.h>
#endif
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
    bbCheck(queue.submit(nullptr) == LockFreeQueueInvalidConfiguration && !queue.take(nullptr, 0),
        "invalid queue input accepted");
    bbCheck(queue.submit(packet) == LockFreeQueueSuccess, "local publish failed");
    char* popped = nullptr;
    bbCheck(queue.take(&popped, 0) && popped == packet && queue.used() == 296, "pop prematurely returned budget");
    queue.release(popped);
    bbCheck(queue.used() == 0, "finished packet retained budget");
    packet = queue.allocate(1024, result);
    bbCheck(packet == nullptr && result == LockFreeQueueInvalidConfiguration,
        "fixed quota admitted an oversized singleton");
    queue.release(packet);
    bbCheck(queue.allocate((std::numeric_limits<size_t>::max)(), result) == nullptr && queue.used() == 0,
        "overflow request reserved capacity");
    packet = queue.allocate(32, result);
    bbCheck(packet != nullptr && queue.submit(packet) == LockFreeQueueSuccess, "cleanup setup failed");
    queue.clear();
    bbCheck(queue.used() == 0 && queue.empty(), "clear leaked packet charge");
    config.m_reserveBytes = (std::numeric_limits<uint64_t>::max)();
    bbCheck(queue.configure(config) && queue.allocate(1, result) == nullptr && queue.used() == 0,
        "memory pressure admitted allocation");
}

/** 对默认分配与预留小包分别验证并发归还、预算与每线程FIFO
@param [in] reserve 预留小包槽数，0使用原分配路径
@param [in] slotBytes 槽字节数，64或128
*/
static void LocalQueueStress(uint32_t reserve, uint32_t slotBytes = 64)
{
    ProcessWorkLocalQueue queue;
    ProcessWorkMemoryConfig config;
    config.m_localQueueBytes = 1024;
    config.m_postBufferReserve = reserve;
    config.m_postBufferSlotBytes = slotBytes;
    bbCheck(queue.configure(config), "stress configure failed");
    std::atomic<bool> cancel(false);
    IpcJobs jobs;
    const int32_t messages = 5000;
    try
    {
        for (int32_t producer = 0; producer < 8; ++producer)
        {
            jobs.add(
                [&queue, &cancel, producer, messages]()
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
                        const int32_t body[2] = {producer, sequence};
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
            bbCheck(body[0] >= 0 && body[0] < 8 && body[1] == expected[body[0]],
                "local queue ordering/duplication failure");
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
    for (uint32_t slotBytes = 64; slotBytes <= 128; slotBytes *= 2)
    {
        ProcessWorkLocalQueue pool;
        ProcessWorkMemoryConfig poolConfig;
        std::shared_ptr<ProcessWorkTestMemoryPolicy> policy(new ProcessWorkTestMemoryPolicy);
        policy->m_increment = 2;
        poolConfig.m_policy = policy;
        poolConfig.m_postBufferMaximum = 2;
        poolConfig.m_postBufferSlotBytes = slotBytes;
        bbCheck(pool.configure(poolConfig), "fixed slot pool configure failed");
        const size_t bytes = slotBytes - 4;
        LockFreeQueueResult result;
        char* oversized = pool.allocate(bytes + 1, result, true);
        bbCheck(
            oversized != nullptr && result == LockFreeQueueSuccess && policy->m_kindCount[ProcessWorkPostBuffers] == 0,
            "oversized body entered fixed slots");
        pool.release(oversized);
        char* first = pool.allocate(bytes, result, true);
        bbCheck(first != nullptr && result == LockFreeQueueSuccess &&
                policy->m_kindCount[ProcessWorkPostBuffers] == 1 && policy->m_kindAdded[ProcessWorkPostBuffers] == 2,
            "exact slot boundary fell back instead of using the configured pool");
        char* second = pool.allocate(bytes, result, true);
        char* fallback = pool.allocate(bytes, result, true);
        bbCheck(second != nullptr && fallback != nullptr && first != second && second != fallback &&
                first != fallback && policy->m_kindCount[ProcessWorkPostBuffers] == 1,
            "full pool exceeded its cap or rejected heap fallback");
        std::memset(first, 11, bytes);
        std::memset(second, 22, bytes);
        std::memset(fallback, 33, bytes);
        bbCheck(first[0] == 11 && first[bytes - 1] == 11 && second[0] == 22 && second[bytes - 1] == 22 &&
                fallback[0] == 33 && fallback[bytes - 1] == 33,
            "slot bodies overlapped");
        pool.release(first);
        pool.release(second);
        pool.release(fallback);
        const uint32_t invalidSizes[] = {0, 63, 65, 127, 129};
        for (size_t i = 0; i < sizeof(invalidSizes) / sizeof(invalidSizes[0]); ++i)
        {
            poolConfig.m_postBufferSlotBytes = invalidSizes[i];
            bbCheck(!pool.configure(poolConfig), "invalid slot size accepted");
        }
        first = pool.allocate(bytes, result, true);
        bbCheck(first != nullptr && result == LockFreeQueueSuccess && policy->m_kindCount[ProcessWorkPostBuffers] == 1,
            "invalid configuration replaced existing pool");
        pool.release(first);
    }
    ProcessWorkLocalQueue queue;
    ProcessWorkMemoryConfig config;
    config.m_postBufferReserve = 7;
    config.m_postBufferMaximum = 7;
    config.m_localQueueBytes = 4096;
    bbCheck(queue.configure(config), "post packet reserve failed");
    // 不足一个热窗口的小池，填满后仍允许合法请求从原分配器取得正文。
    for (int32_t round = 0; round < 80; ++round)
    {
        config.m_postBufferSlotBytes = round < 40 ? 64 : 128;
        bbCheck(queue.configure(config), "packet slot size reconfiguration failed");
        std::vector<char*> packets;
        for (int32_t i = 0; i < 9; ++i)
        {
            LockFreeQueueResult status;
            const size_t bytes = (round & 1) == 0 ? (i == 8 ? 53 : 52) : (i == 8 ? 117 : 116);
            char* packet = queue.allocate(bytes, status, true);
            bbCheck(packet != nullptr && status == LockFreeQueueSuccess, "packet pool fallback rejected valid request");
            std::memset(packet, i, bytes);
            packets.push_back(packet);
        }
        bbCheck(!queue.configure(config), "packet pool replaced while borrowed");
        for (int32_t i = 8; i >= 0; --i)
        {
            const size_t bytes = (round & 1) == 0 ? (i == 8 ? 53 : 52) : (i == 8 ? 117 : 116);
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
    config.m_localQueueBytes = -1;
    bbCheck(queue.configure(config), "prefix recovery setup failed");
    for (int32_t phase = 0; phase < 2; ++phase)
    {
        const int32_t amount = phase == 0 ? 2 : 96;
        for (int32_t i = 0; i < amount; ++i)
        {
            LockFreeQueueResult status;
            char* packet = queue.allocate(sizeof(i), status, true);
            bbCheck(packet != nullptr && status == LockFreeQueueSuccess, "prefix recovery allocation failed");
            std::memcpy(packet, &i, sizeof(i));
            bbCheck(queue.submit(packet) == LockFreeQueueSuccess, "prefix recovery submit failed");
        }
        int32_t expected = 0;
        while (expected < amount)
        {
            char* ready[64] = {};
            size_t taken = 0;
            bbCheck(queue.takePrefix(ready, 64, taken, 0) && taken > 0 &&
                    taken <= static_cast<size_t>((std::min)(64, amount - expected)),
                "prefix delivered an invalid ready count");
            for (size_t i = 0; i < taken; ++i)
            {
                int32_t value = -1;
                std::memcpy(&value, ready[i], sizeof(value));
                bbCheck(value == expected++, "prefix recovery changed FIFO");
            }
            queue.releasePrefix(ready, taken);
        }
        bbCheck(queue.empty(), "prefix recovery retained queue entries");
    }
    config.m_localQueueBytes = 100;
    bbCheck(queue.configure(config), "idle packet reserve must not be counted as message backlog");
    LockFreeQueueResult oversized;
    bbCheck(queue.allocate(1, oversized) == nullptr && oversized == LockFreeQueueInvalidConfiguration,
        "prepared idle cache bypassed fixed message quota");
    config.m_localQueueBytes = 4096;
    config.m_reserveBytes = UINT64_MAX;
    bbCheck(!queue.configure(config), "queue preparation ignored system reserve");
    LocalQueueStress(16);
    LocalQueueStress(16, 128);
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
    bbCheck(ipc.tryPost(bbPid(), body.data(), 16, static_cast<MessageType>(1)) == PROCESS_WORK_POSTED,
        "first tryPost failed");
    bbCheck(ipc.postPendingBytes() == 296, "post charge incorrect");
    bbCheck(ipc.tryPost(bbPid(), body.data(), 16, static_cast<MessageType>(2)) == PROCESS_WORK_POST_CAPACITY,
        "over-budget tryPost must report capacity, not competition");
    bbCheck(ipc.tryPost(0, body.data(), 16) == PROCESS_WORK_POST_INVALID &&
            ipc.tryPost(std::string(), body.data(), 16) == PROCESS_WORK_POST_INVALID,
        "invalid tryPost admitted");
    bbCheck(
        !ipc.configurePostMemory(memory) && ipc.postPendingBytes() == 296, "failed post/configuration changed charge");
    bbCheck(
        ipc.tryPost(bbPid(), body.data(), 16, static_cast<MessageType>(3), true, false) == PROCESS_WORK_POST_CAPACITY,
        "contention retry must preserve capacity result");
    bbCheck(ipc.postResult(bbPid(), body.data(), 16, static_cast<MessageType>(3)) == PROCESS_WORK_POST_UNAVAILABLE &&
            !ipc.post(bbPid(), body, static_cast<MessageType>(3)),
        "post cannot wait for an unstarted consumer");
    bbCheck(ipc.postPendingBytes() == 296, "failed compatibility post changed pending bytes");
    scope.start(&log, receiveName("post-budget"), 64, 2);
    bbCheck(ipc.initPostThread(), "post worker initialization failed");
    ipc.waitForPostEnd();
    waitPackets(log, 1);
    bbCheck(log.snapshot()[0].m_type == 1 && log.snapshot()[0].m_body == body && ipc.postPendingBytes() == 0,
        "failed post delivered or completed charge retained");
    ipc.uninitPostThread();
    ProcessWorkMemoryConfig denied;
    // 不启动消费者，填完已有节点。回调返回0拒绝一次节点池增长，原有队列仍可排空。
    const std::shared_ptr<std::atomic<bool>> asked = std::make_shared<std::atomic<bool>>(false);
    denied.m_queueGrow = [asked](const LockFreeQueueGrowContext&) -> size_t
    {
        asked->store(true);
        return 0;
    };
    bbCheck(ipc.configurePostMemory(denied), "refusing queue policy rejected");
    int32_t admitted = 0;
    int32_t status = PROCESS_WORK_POSTED;
    while (admitted < 256 && status == PROCESS_WORK_POSTED)
    {
        status = ipc.tryPost(bbPid(), "node", 4, static_cast<MessageType>(admitted + 10), true, false);
        if (status == PROCESS_WORK_POSTED)
        {
            ++admitted;
        }
    }
    bbCheck(asked->load() && admitted > 0 && admitted < 256 && status == PROCESS_WORK_POST_CAPACITY,
        "declined node growth did not report recoverable capacity");
    bbCheck(ipc.initPostThread(), "consumer failed after declined growth");
    ipc.waitForPostEnd();
    waitPackets(log, static_cast<size_t>(admitted + 1));
    ipc.uninitPostThread();
    const std::vector<ReceivedPacket> deliveries = log.snapshot();
    for (int32_t i = 0; i < admitted; ++i)
    {
        bbCheck(deliveries[i + 1].m_type == i + 10 && deliveries[i + 1].m_body == "node",
            "declined growth changed admitted FIFO");
    }
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
    bbCheck(received[0].m_type == 1 && received[1].m_type == 2 && received[2].m_type == 3 && received[3].m_type == 4 &&
            received[0].m_body == "123456" && received[1].m_body.empty() && received[2].m_body == "123456" &&
            received[3].m_body == "123456",
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
            jobs.add(
                [&ipc, &ready, &go, thread]()
                {
                    ++ready;
                    while (!go.load(std::memory_order_acquire))
                    {
                        std::this_thread::yield();
                    }
                    for (int32_t sequence = 0; sequence < 5000; ++sequence)
                    {
                        bbCheck(ipc.post(bbPid(), "123456", 6, static_cast<MessageType>(thread * 5000 + sequence)),
                            "ordinary post rejected");
                    }
                });
        }
        bbWait(
            [&ready]()
            {
                return ready.load() == 32;
            },
            "ordinary post producer barrier");
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
    bbCheck(ipc.initPostThread(), "post worker initialization failed");
    ipc.waitForPostEnd();
    waitPackets(log, 32 * count);
    const std::vector<ReceivedPacket> packets = log.snapshot();
    int32_t expected[32] = {};
    for (size_t i = 0; i < packets.size(); ++i)
    {
        const int32_t thread = packets[i].m_type / count;
        bbCheck(thread >= 0 && thread < 32 && packets[i].m_type % count == expected[thread] &&
                packets[i].m_pid == bbPid() && packets[i].m_body == "123456",
            "ordinary post reordered or corrupted fixed payload");
        ++expected[thread];
    }
    bbCheck(
        log.m_errors == 0 && packets.size() == 32 * count && ipc.postPendingBytes() == 0, "ordinary post incomplete");
}

void testProcessWorkSlowCallback()
{
    ProcessWork& ipc = ProcessWork::instance();
    PacketLog log;
    std::atomic<bool> entered(false), resume(false);
    IpcScope scope;
    log.m_action = [&entered, &resume](const ReceivedPacket& packet)
    {
        if (packet.m_type == 0)
        {
            entered.store(true);
            bbWait(
                [&resume]()
                {
                    return resume.load();
                },
                "release deliberately slow callback", 10000);
        }
    };
    ProcessWorkMemoryConfig memory;
    memory.m_localQueueBytes = 300;
    ipc.addProcessReceiveCallback(&log);
    bbCheck(ipc.setReceiveName(receiveName("slow-callback")) && ipc.initReceive(28, 2, 0, 0, 0, 0, memory),
        "slow callback setup failed");
    const std::string body(16, 'x');
    try
    {
        bbCheck(ipc.send(bbPid(), body, static_cast<MessageType>(0)), "backpressure send rejected");
        bbWait(
            [&entered]()
            {
                return entered.load();
            },
            "slow callback entered");
        bbCheck(ipc.receivePendingBytes() == 293, "executing callback not charged");
        for (int32_t type = 1; type <= 2; ++type)
        {
            bbWait(
                [&ipc, &body, type]()
                {
                    const int32_t result = ipc.trySend(bbPid(), body.data(), 16, static_cast<MessageType>(type));
                    bbCheck(result == PROCESS_WORK_SENT || result == PROCESS_WORK_BUSY ||
                            result == PROCESS_WORK_CAPACITY || result == PROCESS_WORK_RESOURCE,
                        "shared admission failed");
                    return result == PROCESS_WORK_SENT;
                },
                "shared slot returned by reader");
        }
        bbCheck(ipc.trySend(bbPid(), body.data(), 16, static_cast<MessageType>(3)) == PROCESS_WORK_CAPACITY,
            "slow callback bypassed shared pressure");
        bbSleep(100);
        bbCheck(ipc.receivePendingBytes() == 293 && ipc.receiveBufferCount() == 2,
            "slow callback caused uncontrolled local allocation");
        resume.store(true);
        waitPackets(log, 3);
        bbWait(
            [&ipc]()
            {
                return ipc.receivePendingBytes() == 0;
            },
            "callback charges returned");
        const std::vector<ReceivedPacket> packets = log.snapshot();
        for (int32_t i = 0; i < 3; ++i)
        {
            bbCheck(packets[i].m_type == i && packets[i].m_body == body, "pressure retry changed payload/order");
        }
    }
    catch (...)
    {
        resume.store(true);
        throw;
    }
}

#ifdef _WIN32
/** 在持续满载下核对八生产者和消费者每秒都有进展，并核对停止后的精确交付
*/
static void CheckLocalQueueProgress()
{
    ProcessWorkLocalQueue queue;
    ProcessWorkMemoryConfig config;
    config.m_localQueueBytes = 4096;
    config.m_localQueueInitialCapacity = 512;
    config.m_postBufferReserve = 512;
    config.m_postBufferMaximum = 512;
    bbCheck(queue.configure(config), "limited CPU queue initialization");
    std::atomic<bool> stop(false), start(false);
    std::atomic<int32_t> ready(0);
    std::atomic<int32_t> sent[8];
    int32_t received[8] = {}, previous[8] = {};
    IpcJobs jobs;
    try
    {
        for (int32_t stream = 0; stream < 8; ++stream)
        {
            sent[stream].store(0);
            // stream标识唯一生产流；每条消息有独立序号，停止时归还尚未提交的正文。
            jobs.add(
                [&queue, &stop, &start, &ready, &sent, stream]()
                {
                    ready.fetch_add(1);
                    while (!start.load() && !stop.load())
                    {
                        std::this_thread::yield();
                    }
                    int32_t sequence = 0;
                    while (!stop.load())
                    {
                        LockFreeQueueResult result;
                        char* packet = queue.allocate(8, result, true);
                        if (packet == nullptr)
                        {
                            bbCheck(result == LockFreeQueueCapacityLimit || result == LockFreeQueueTemporaryUnavailable,
                                "limited CPU allocation failed unexpectedly");
                            std::this_thread::yield();
                            continue;
                        }
                        const int32_t body[2] = {stream, sequence};
                        std::memcpy(packet, body, sizeof(body));
                        bool committed = false;
                        while (!stop.load())
                        {
                            result = queue.submit(packet);
                            if (result == LockFreeQueueSuccess)
                            {
                                committed = true;
                                break;
                            }
                            if (result != LockFreeQueueCapacityLimit && result != LockFreeQueueTemporaryUnavailable)
                            {
                                queue.release(packet);
                                bbCheck(false, "limited CPU publish failed unexpectedly");
                            }
                            std::this_thread::yield();
                        }
                        // 停止标记不撤销已经提交的所有权；仅未提交的正文由生产者归还。
                        if (committed)
                        {
                            ++sequence;
                            sent[stream].store(sequence);
                        }
                        else
                        {
                            queue.release(packet);
                        }
                    }
                });
        }
        bbWait(
            [&ready]()
            {
                return ready.load() == 8;
            },
            "limited CPU producers ready");
        start.store(true);
        const uint64_t started = bbNow();
        uint64_t checkAt = started + 1000;
        while (bbNow() - started < 3100)
        {
            char* packet = nullptr;
            if (queue.take(&packet, 10))
            {
                int32_t body[2] = {};
                std::memcpy(body, packet, sizeof(body));
                queue.release(packet);
                bbCheck(body[0] >= 0 && body[0] < 8 && body[1] == received[body[0]], "limited CPU FIFO violation");
                ++received[body[0]];
            }
            if (bbNow() >= checkAt)
            {
                for (int32_t stream = 0; stream < 8; ++stream)
                {
                    bbCheck(received[stream] > previous[stream],
                        "a local producer or consumer made no progress in a full second");
                    previous[stream] = received[stream];
                }
                checkAt += 1000;
            }
        }
        stop.store(true);
        jobs.drain();
        char* packet = nullptr;
        while (queue.take(&packet, 0))
        {
            int32_t body[2] = {};
            std::memcpy(body, packet, sizeof(body));
            queue.release(packet);
            bbCheck(body[0] >= 0 && body[0] < 8 && body[1] == received[body[0]], "limited CPU final FIFO violation");
            ++received[body[0]];
        }
        for (int32_t stream = 0; stream < 8; ++stream)
        {
            bbCheck(received[stream] == sent[stream].load(), "limited CPU delivery count mismatch");
        }
        bbCheck(!queue.hasPending(), "limited CPU retained local ownership");
    }
    catch (...)
    {
        stop.store(true);
        throw;
    }
}

void testProcessWorkPostLimitedCpu()
{
    DWORD_PTR original = 0, system = 0;
    bbCheck(GetProcessAffinityMask(GetCurrentProcess(), &original, &system) != FALSE && original != 0,
        "cannot read test CPU affinity");
    const DWORD_PTR selected = original & (~original + 1);
    bbCheck(SetProcessAffinityMask(GetCurrentProcess(), selected) != FALSE, "cannot limit test to one CPU");
    try
    {
        CheckLocalQueueProgress();
    }
    catch (...)
    {
        SetProcessAffinityMask(GetCurrentProcess(), original);
        throw;
    }
    bbCheck(SetProcessAffinityMask(GetCurrentProcess(), original) != FALSE, "cannot restore test CPU affinity");
}
#endif