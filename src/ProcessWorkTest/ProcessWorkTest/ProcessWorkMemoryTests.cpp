#include "ProcessWorkMemoryTests.h"
#include "ProcessWorkTest.h"
#include "ProcessWork/ProcessWorkTime.h"
#include <limits>
#include <cstring>
#include <stdexcept>
#include <thread>

/** 按库实际使用的单调时钟跨过资源重试窗口，不能把sleep时长当作粗粒度时钟的进度
@param [in] milliseconds 需要经过的库时钟毫秒数
*/
static void WaitMemoryRetry(uint32_t milliseconds)
{
    const uint64_t started = ProcessWorkTime::milliseconds();
    // 返回true时库的重试窗口已经到期；bbWait另有测试总超时。
    bbWait([started, milliseconds]()
    {
        return ProcessWorkTime::milliseconds() - started >= milliseconds;
    }, "memory retry clock window");
}

ProcessWorkTestMemoryPolicy::ProcessWorkTestMemoryPolicy() :
m_calls(0), m_notifications(0), m_added(0), m_increment(0), m_refuse(false), m_throwGrow(false), m_throwNotification(false)
{
    for (size_t i = 0; i < 3; ++i)
    {
        m_kindCount[i].store(0);
        m_kindAdded[i].store(0);
    }
}

uint64_t ProcessWorkTestMemoryPolicy::grow(const ProcessWorkMemoryGrowContext& context)
{
    m_calls.fetch_add(1);
    if (m_throwGrow)
    {
        throw std::runtime_error("test policy failure");
    }
    return m_refuse ? 0 : m_increment != 0 ? m_increment : ProcessWorkMemoryPolicy::grow(context);
}

void ProcessWorkTestMemoryPolicy::grown(const ProcessWorkMemoryGrowth& growth)
{
    bbCheck(growth.m_currentCapacity > growth.m_previousCapacity &&
        growth.m_addedCapacity == growth.m_currentCapacity - growth.m_previousCapacity, "invalid growth fact");
    m_added.fetch_add(growth.m_addedCapacity);
    m_notifications.fetch_add(1);
    m_kindCount[growth.m_kind].fetch_add(1);
    m_kindAdded[growth.m_kind].fetch_add(growth.m_addedCapacity);
    if (m_throwNotification)
    {
        throw std::runtime_error("test observer failure");
    }
}

ProcessWorkTestGrowthObserver::ProcessWorkTestGrowthObserver(bool& fail) :
m_fail(&fail)
{

}

ProcessWorkTestGrowthObserver::ProcessWorkTestGrowthObserver(const ProcessWorkTestGrowthObserver& other) :
m_fail(other.m_fail)
{
    if (*m_fail)
    {
        throw std::runtime_error("observer copy failure");
    }
}

void ProcessWorkTestGrowthObserver::operator()(size_t added, uint64_t sequence) const
{
    (void)added;
    (void)sequence;
}

int32_t runProcessWorkQuotaProbe(int32_t pid)
{
    ProcessWork& ipc = ProcessWork::instance();
    const std::string rejected(24, 'x');
    const std::string accepted(23, 'x');
    bbCheck(ipc.trySend(pid, rejected.data(), 24, static_cast<MessageType>(5), true, true) == PROCESS_WORK_TOO_LARGE &&
        !ipc.send(pid, rejected, static_cast<MessageType>(5)), "cross-process oversized request did not terminate");
    bbCheck(ipc.send(pid, accepted, static_cast<MessageType>(4)), "cross-process exact quota rejected");
    ipc.clear();
    return 0;
}

void testProcessWorkFixedBudget()
{
    ProcessWorkMemoryConfig config;
    ProcessWorkLocalQueue queue;
    std::shared_ptr<ProcessWorkTestMemoryPolicy> policy(new ProcessWorkTestMemoryPolicy);
    config.m_policy = policy;
    config.m_postBufferMaximum = 0;
    config.m_localQueueBytes = 592;
    bbCheck(queue.configure(config), "fixed quota setup");
    LockFreeQueueResult result;
    char* first = queue.allocate(32, result, true);
    char* second = queue.allocate(32, result, true);
    bbCheck(first != nullptr && second != nullptr && queue.used() == 592, "exact quota boundary");
    bbCheck(queue.allocate(1, result, true) == nullptr && result == LockFreeQueueCapacityLimit,
        "fixed quota grew or returned Busy");
    queue.release(first);
    queue.release(second);
    bbCheck(queue.allocate(329, result, true) == nullptr && result == LockFreeQueueInvalidConfiguration && !queue.hasPending(),
        "intrinsically oversized request must not wait forever");
    bbCheck(policy->m_calls == 0 && policy->m_notifications == 0, "quota invoked storage growth callback");
    config.m_localQueueBytes = -1;
    bbCheck(queue.configure(config), "unlimited quota setup");
    // 原128MiB不再是隐含边界，且不建立按消息累加的字节账本。
    first = queue.allocate(128 * 1024 * 1024 + 1, result, true);
    bbCheck(first != nullptr && result == LockFreeQueueSuccess && queue.used() == 0 && queue.hasPending(),
        "unlimited mode retained hidden limit or byte ledger");
    bbCheck(!queue.configure(config), "unpublished unlimited allocation lost lifetime protection");
    queue.release(first);
    bbCheck(!queue.hasPending() && queue.configure(config), "unlimited lifetime did not drain");
    ProcessWorkReceiveQueue receive;
    bbCheck(receive.configure(config), "unlimited receive setup");
    const std::string body(18, 'x');
    bbCheck(receive.append(body.data(), body.size()) && receive.used() == 0 && receive.hasPending() &&
        !receive.configure(config), "unpublished receive lifetime missing");
    bbCheck(receive.flush(), "unlimited receive flush");
    char* packet = nullptr;
    bbCheck(receive.take(&packet, 0) && receive.hasPending() && !receive.configure(config), "callback lifetime missing");
    receive.release(packet);
    bbCheck(!receive.hasPending() && receive.configure(config), "unlimited callback lifetime leaked");
    config.m_localQueueBytes = 282;
    bbCheck(receive.configure(config) && !receive.append(body.data(), body.size()) && !receive.hasPending(),
        "receive oversized packet bypassed fixed quota");
    config.m_localQueueBytes = 283;
    bbCheck(receive.configure(config) && receive.append(body.data(), body.size()) && receive.used() == 283,
        "receive exact limit rejected");
    receive.clear();
    config.m_localQueueBytes = 0;
    bbCheck(!queue.configure(config) && !receive.configure(config), "zero quota accepted");
    config.m_localQueueBytes = -2;
    bbCheck(!queue.configure(config) && !receive.configure(config), "invalid negative quota accepted");
    ProcessWork& ipc = ProcessWork::instance();
    PacketLog log;
    IpcScope scope;
    config.m_localQueueBytes = 300;
    bbCheck(ipc.configurePostMemory(config), "public fixed post setup");
    const std::string tooLarge(32, 'x');
    bbCheck(ipc.tryPost(bbPid(), tooLarge.data(), 32, static_cast<MessageType>(1), true, true) == PROCESS_WORK_POST_INVALID &&
        !ipc.post(bbPid(), tooLarge, static_cast<MessageType>(1)), "impossible public post did not return terminal error");
    bbCheck(ipc.addProcessReceiveCallback(&log) && ipc.initReceive(64, 2, 0, 0, 0, 0, config),
        "valid quota smaller than maximum packet incorrectly rejected configuration");
    const std::string rejected(24, 'x');
    const std::string accepted(23, 'x');
    bbCheck(ipc.trySend(bbPid(), rejected.data(), 24, static_cast<MessageType>(2), true, true) == PROCESS_WORK_TOO_LARGE &&
        !ipc.send(bbPid(), rejected, static_cast<MessageType>(2)), "remote fixed quota was not checked before admission");
    bbCheck(ipc.send(bbPid(), accepted, static_cast<MessageType>(3)), "remote exact single-message quota rejected");
    waitPackets(log, 1);
    bbCheck(log.snapshot()[0].type == 3 && log.snapshot()[0].body == accepted, "failed request committed or blocked next packet");
    BlackBoxChild child;
    std::vector<std::string> arguments;
    arguments.push_back("--quota-probe");
    arguments.push_back(bbNumber(bbPid()));
    child.start(arguments, bbTempPath("-quota-probe.log"));
    bbCheck(child.wait(10000) == 0, "separate-process quota probe failed");
    waitPackets(log, 2);
    bbCheck(log.snapshot()[1].type == 4 && log.snapshot()[1].body == accepted, "cross-process quota lost or duplicated messages");
    scope.close();
    config.m_localQueueBytes = 1;
    bbCheck(ipc.initReceive(64, 2, 0, 0, 0, 0, config), "positive tiny quota should be valid configuration");
    bbCheck(ipc.sendResult(bbPid(), nullptr, 0) == PROCESS_WORK_TOO_LARGE, "empty message bypassed impossible remote quota");
    scope.close();
    bbCheck(ipc.configurePostMemory(ProcessWorkMemoryConfig()), "restore default post quota");
}

void testProcessWorkMemoryConfiguration()
{
    ProcessWorkMemoryConfig config;
    bbCheck(config.m_localQueueBytes == -1 && config.m_queuePool.maximumCapacity == -1 &&
        config.m_freeQueuePool.maximumCapacity == -1, "default local capacity must be unlimited");
    bbCheck(config.setLocalQueueLimitMB(128) && config.m_localQueueBytes == 128LL * 1024 * 1024,
        "MB configuration did not convert to bytes");
    bbCheck(!config.setLocalQueueLimitMB(0) && !config.setLocalQueueLimitMB(-2) &&
        !config.setLocalQueueLimitMB(INT64_MAX / (1024 * 1024) + 1) && config.m_localQueueBytes == 128LL * 1024 * 1024,
        "invalid MB value changed previous configuration");
    bbCheck(config.setLocalQueueLimitMB(INT64_MAX / (1024 * 1024)) &&
        config.m_localQueueBytes == (INT64_MAX / (1024 * 1024)) * (1024 * 1024), "largest valid MB value overflowed");
    bbCheck(config.setLocalQueueLimitMB(-1) && config.m_localQueueBytes == -1, "MB unlimited setting rejected");
    std::shared_ptr<ProcessWorkTestMemoryPolicy> policy(new ProcessWorkTestMemoryPolicy);
    config.m_policy = policy;
    ProcessWorkLocalQueue queue;
    bbCheck(queue.configure(config), "post dynamic setup");
    std::vector<char*> packets;
    for (int32_t i = 0; i < 32; ++i)
    {
        LockFreeQueueResult result;
        char* packet = queue.allocate(32, result, true);
        bbCheck(packet != nullptr && result == LockFreeQueueSuccess, "post should grow beyond initial byte budget");
        packets.push_back(packet);
    }
    bbCheck(queue.used() == 0 && queue.hasPending(),
        "unlimited allocations were charged or lost lifetime protection");
    for (size_t i = 0; i < packets.size(); ++i)
    {
        queue.release(packets[i]);
    }
    bbCheck(queue.used() == 0, "post growth leaked charge");
    config.m_localQueueBytes = 600;
    bbCheck(queue.configure(config), "finite post reconfigure");
    LockFreeQueueResult result;
    char* first = queue.allocate(32, result);
    char* second = queue.allocate(32, result);
    bbCheck(first != nullptr && second != nullptr, "finite post first reservations");
    bbCheck(queue.allocate(32, result) == nullptr && result == LockFreeQueueCapacityLimit && queue.used() == 592,
        "finite post ignored maximum or misclassified it as Busy");
    queue.release(first);
    queue.release(second);
    config.m_localQueueBytes = -1;
    ProcessWorkReceiveQueue receive;
    bbCheck(receive.configure(config), "receive dynamic setup");
    const std::string packet(18, 'x');
    const uint64_t prior = policy->m_notifications.load();
    const uint64_t priorCalls = policy->m_calls.load();
    policy->m_throwGrow = true;
    bbCheck(!receive.append(packet.data(), packet.size()) && !receive.hasPending() && policy->m_notifications == prior &&
        policy->m_calls == priorCalls + 1,
        "real receive growth exception committed a packet or emitted success");
    WaitMemoryRetry(config.m_retryMilliseconds);
    policy->m_throwGrow = false;
    policy->m_refuse = true;
    bbCheck(!receive.append(packet.data(), packet.size()) && !receive.hasPending() && policy->m_notifications == prior &&
        policy->m_calls == priorCalls + 2,
        "real receive growth refusal changed ownership");
    WaitMemoryRetry(config.m_retryMilliseconds);
    policy->m_refuse = false;
    bbCheck(receive.append(packet.data(), packet.size()) && receive.flush() && policy->m_calls == priorCalls + 3,
        "receive dynamic append");
    char* read = nullptr;
    bbCheck(receive.take(&read, 0) && std::memcmp(read, packet.data(), packet.size()) == 0, "receive growth altered bytes");
    receive.release(read);
    bbCheck(receive.used() == 0 && policy->m_notifications > prior, "receive real allocation was not observed");
    IpcScope scope;
    ProcessWork& ipc = ProcessWork::instance();
    ProcessWorkConfig components;
    components.m_postThread.m_maximumTasks = 0;
    bbCheck(ipc.configure(components), "component setup");
    bbCheck(!ipc.initPostThread(), "post startup ignored task admission limit");
    components.m_postThread.m_maximumTasks = -1;
    components.m_receiveThread.m_maximumTasks = 0;
    bbCheck(ipc.configure(components), "component retry setup");
    bbCheck(!ipc.initReceive(76, 4, 0, 0, 0, 0), "receive startup ignored task admission limit");
    components.m_receiveThread.m_maximumTasks = -1;
    components.m_sendChannels.nodes.initialCapacity = 1;
    components.m_sendChannels.nodes.maximumCapacity = 0;
    bbCheck(!ipc.configure(components), "invalid native Map config was silently ignored");
    components.m_sendChannels.nodes.maximumCapacity = -1;
    bbCheck(ipc.configure(components) && ipc.initReceive(76, 4, 0, 0, 0, 0) && ipc.initPostThread(), "component configuration recovery failed");
    scope.close();
}

void testProcessWorkNativeGrowth()
{
    bool copyFails = false;
    LockFreeQueuePoolConfig failingConfig;
    failingConfig.initialCapacity = 2;
    failingConfig.m_onGrowth = ProcessWorkTestGrowthObserver(copyFails);
    LockFreeQueue<int32_t> unchanged(false);
    copyFails = true;
    bbCheck(unchanged.configurePool(failingConfig) == LockFreeQueueAllocationFailure,
        "queue observer copy exception escaped configuration");
    copyFails = false;
    bbCheck(unchanged.tryInit() == LockFreeQueueSuccess && unchanged.poolStatistics().currentCapacity == 64,
        "failed configuration partly replaced original pool");
    std::atomic<uint64_t> calls(0), added(0);
    // 已提交后的观察异常必须被隔离，不能令同一条消息被外部重复发送。
    const std::function<void(size_t, uint64_t)> observe = [&calls, &added](size_t count, uint64_t sequence)
    {
        if (count != 0 && sequence != 0)
        {
            calls.fetch_add(1);
            added.fetch_add(count);
        }
        throw std::runtime_error("growth observer");
    };
    LockFreeQueuePoolConfig queueConfig;
    queueConfig.initialCapacity = 4;
    queueConfig.m_onGrowth = observe;
    LockFreeQueue<int32_t> queue(false);
    bbCheck(queue.configurePool(queueConfig) == LockFreeQueueSuccess && queue.tryInit() == LockFreeQueueSuccess,
        "native queue setup");
    bbCheck(calls == 0, "initial queue reserve is not growth");
    for (int32_t i = 0; i < 256; ++i)
    {
        bbCheck(queue.tryPush(i, true, false) == LockFreeQueueSuccess, "queue observer prevented commit");
    }
    const LockFreeQueuePoolStatistics queueStats = queue.poolStatistics();
    bbCheck(calls != 0 && calls == queueStats.successfulGrowCount, "queue growth event missing/duplicate");
    for (int32_t i = 0; i < 256; ++i)
    {
        int32_t value = -1;
        bbCheck(queue.pop(&value) && value == i, "queue growth altered FIFO");
    }
    calls = 0;
    added = 0;
    LockFreeMapConfig mapConfig;
    mapConfig.nodes.initialCapacity = 2;
    mapConfig.values.initialCapacity = 2;
    mapConfig.nodes.m_onGrowth = observe;
    LockFreeMap<int32_t, int32_t> map;
    bbCheck(map.lockFreeMapConfigure(mapConfig) == LOCK_FREE_MAP_OK && calls == 0, "map setup event");
    for (int32_t i = 0; i < 64; ++i)
    {
        bbCheck(map.tryInsert(i, i, true, false) == LOCK_FREE_MAP_COMMITTED, "map observer prevented commit");
    }
    bbCheck(calls != 0 && calls == map.lockFreeMapStats().nodes.successfulGrowCount, "map growth event mismatch");
    calls = 0;
    added = 0;
    LockFreeHashMapPoolConfig hashConfig;
    hashConfig.initialCapacity = 2;
    hashConfig.m_onGrowth = observe;
    LockFreeHashMap<int32_t, int32_t> hash;
    bbCheck(hash.configurePool(hashConfig).m_status == LockFreeHashMapOk && calls == 0, "hash setup event");
    for (int32_t i = 0; i < 64; ++i)
    {
        bbCheck(hash.tryInsert(i, i, true, false).m_committed, "hash observer prevented commit");
    }
    LockFreeHashMapPoolStats hashStats;
    bbCheck(hash.poolStats(hashStats).m_status == LockFreeHashMapOk && calls != 0 && calls == hashStats.successfulGrowCount,
        "hash growth event mismatch");
}

void testProcessWorkPacketGrowth()
{
    ProcessWorkLocalQueue queue;
    ProcessWorkMemoryConfig config;
    std::shared_ptr<ProcessWorkTestMemoryPolicy> policy(new ProcessWorkTestMemoryPolicy);
    config.m_policy = policy;
    config.m_postBufferReserve = 2;
    config.m_postBufferMaximum = 9;
    policy->m_increment = UINT64_MAX;
    policy->m_throwNotification = true;
    bbCheck(queue.configure(config), "packet pool finite configure");
    std::vector<char*> held;
    for (int32_t i = 0; i < 20; ++i)
    {
        LockFreeQueueResult result;
        char* packet = queue.allocate(18, result, true);
        bbCheck(packet != nullptr && result == LockFreeQueueSuccess, "finite cache fallback failed");
        std::memset(packet, i, 18);
        held.push_back(packet);
    }
    bbCheck(policy->m_kindCount[ProcessWorkPostBuffers] == 1 && policy->m_kindAdded[ProcessWorkPostBuffers] == 7,
        "cache growth exceeded cap or initial reserve notified");
    for (size_t i = 0; i < held.size(); ++i)
    {
        bbCheck(held[i][0] == static_cast<char>(i) && held[i][17] == static_cast<char>(i), "cache slots overlap");
        queue.release(held[i]);
    }
    config.m_postBufferMaximum = -1;
    policy.reset(new ProcessWorkTestMemoryPolicy);
    config.m_policy = policy;
    bbCheck(queue.configure(config), "packet concurrent setup");
    std::atomic<bool> start(false);
    std::atomic<uint32_t> finished(0), errors(0);
    std::vector<std::thread> threads;
    for (int32_t producer = 0; producer < 32; ++producer)
    {
        threads.push_back(std::thread([&queue, &start, &finished, &errors, producer]()
        {
            while (!start.load())
            {
                std::this_thread::yield();
            }
            for (int32_t round = 0; round < 128; ++round)
            {
                char* packets[16] = {};
                for (size_t i = 0; i < 16; ++i)
                {
                    LockFreeQueueResult result;
                    packets[i] = queue.allocate(18, result, true);
                    if (packets[i] == nullptr || result != LockFreeQueueSuccess)
                    {
                        errors.fetch_add(1);
                    }
                    else
                    {
                        std::memset(packets[i], producer, 18);
                    }
                }
                std::this_thread::yield();
                for (size_t i = 0; i < 16; ++i)
                {
                    if (packets[i] != nullptr && (packets[i][0] != producer || packets[i][17] != producer))
                    {
                        errors.fetch_add(1);
                    }
                    queue.release(packets[i]);
                }
            }
            finished.fetch_add(1);
        }));
    }
    start = true;
    for (size_t i = 0; i < threads.size(); ++i)
    {
        threads[i].join();
    }
    bbCheck(finished == 32 && errors == 0 && queue.used() == 0, "concurrent cache growth stalled/corrupted/leaked");
    bbCheck(policy->m_kindCount[ProcessWorkPostBuffers] != 0, "unlimited cache did not grow");
}

void testProcessWorkSharedGrowth()
{
    PublicChannelScope channel(4, 0, 2, 0, 1);
    channel.sender.close();
    channel.receiver.stop();
    channel.receiver.close();
    ProcessWorkMemoryConfig config;
    std::shared_ptr<ProcessWorkTestMemoryPolicy> policy(new ProcessWorkTestMemoryPolicy);
    policy->m_increment = UINT64_MAX;
    policy->m_throwNotification = true;
    config.m_policy = policy;
    bbCheck(channel.receiver.openReceiver(76, 4, 0, 2, 0, 1, config), "shared policy open");
    channel.receiver.publish();
    bbCheck(channel.sender.openSender(bbPid()), "shared policy sender");
    channel.pump();
    bbCheck(policy->m_notifications == 0, "shared initial reserve was notified as growth");
    for (int32_t i = 0; i < 4; ++i)
    {
        bbCheck(channel.sender.trySend("x", 1, i) == PROCESS_WORK_SENT, "shared initial capacity");
    }
    bbCheck(channel.sender.trySend("x", 1, 4) == PROCESS_WORK_CAPACITY, "shared growth request committed twice");
    const uint64_t started = ProcessWorkTime::milliseconds();
    bbWait([started]() { return ProcessWorkTime::milliseconds() - started >= 25; }, "shared maintenance window");
    bbWait([&channel]() { channel.pump(); return channel.receiver.bufferCount() == 6; }, "shared growth cap clipping");
    bbWait([&channel]() { channel.pump(); return channel.delivered.size() == 4; }, "shared growth drain");
    bbCheck(policy->m_kindCount[ProcessWorkSharedBuffers] == 1 && policy->m_kindAdded[ProcessWorkSharedBuffers] == 2,
        "shared success notification mismatch");
    for (size_t i = 0; i < channel.delivered.size(); ++i)
    {
        bbCheck(channel.delivered[i] == static_cast<int32_t>(i), "shared growth lost/reordered data");
    }
}