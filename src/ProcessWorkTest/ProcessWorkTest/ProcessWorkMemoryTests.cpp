#include "ProcessWorkMemoryTests.h"
#include "ProcessWorkTest.h"
#include "ProcessWork/ProcessWorkTime.h"
#include "CSystem/CSystemAPI.h"
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
    bbWait(
        [started, milliseconds]()
        {
            return ProcessWorkTime::milliseconds() - started >= milliseconds;
        },
        "memory retry clock window");
}

ProcessWorkTestMemoryPolicy::ProcessWorkTestMemoryPolicy() :
m_calls(0), m_notifications(0), m_added(0), m_increment(0), m_refuse(false), m_throwGrow(false),
m_throwNotification(false)
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
    // 通知异常会被生产库隔离；校验失败必须另行留痕，不能靠抛异常报告。
    if (static_cast<size_t>(growth.m_kind) >= 3 || growth.m_currentCapacity <= growth.m_previousCapacity ||
        growth.m_addedCapacity != growth.m_currentCapacity - growth.m_previousCapacity)
    {
        bbCleanupFailure("invalid growth fact");
        return;
    }
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
            !ipc.send(pid, rejected, static_cast<MessageType>(5)),
        "cross-process oversized request did not terminate");
    bbCheck(ipc.send(pid, accepted, static_cast<MessageType>(4)), "cross-process exact quota rejected");
    ipc.clear();
    return 0;
}

int32_t runProcessWorkNotificationFailure()
{
    Semaphore notification;
    bbCheck(
        notification.initializationResult().status == SEMAPHORE_NO_MEMORY, "notification failure injection missing");
    ProcessWorkMemoryConfig config;
    ProcessWorkLocalQueue post;
    ProcessWorkReceiveQueue receive;
    bbCheck(
        !post.configure(config) && !receive.configure(config), "failed notification backend accepted configuration");
    ProcessWork& ipc = ProcessWork::instance();
    IpcScope scope;
    bbCheck(!ipc.configurePostMemory(config) && !ipc.initPostThread() && !ipc.initReceive(64, 2),
        "failed notification backend published worker");
    bbCheck(CTaskThreadManager::instance().count() == 0 && ipc.receiveBufferCount() == 0,
        "notification failure retained worker or receiver");
    return 0;
}

void testProcessWorkConfigurationCopyFailure()
{
    bool copyFails = false;
    ProcessWorkMemoryConfig config;
    config.m_queuePool.m_onGrowth = ProcessWorkTestGrowthObserver(copyFails);
    config.m_freeQueuePool.m_onGrowth = ProcessWorkTestGrowthObserver(copyFails);
    LockFreeQueuePoolConfig merged;
    ProcessWorkChannel receiver;
    copyFails = true;
    bbCheck(!ProcessWorkMemory::queueConfig(config, false, merged) && !ProcessWorkMemory::queueConfig(config, true, merged),
        "throwing growth observer escaped or merged successfully");
    bbCheck(!receiver.openReceiver(76, 2, 0, 0, 0, 0, config), "failed configuration copy reported success");
    bbCheck(!receiver.ready() && receiver.bufferCount() == 0, "failed configuration published receiver");
    copyFails = false;
    bbCheck(ProcessWorkMemory::queueConfig(config, false, merged) && merged.m_onGrowth &&
        ProcessWorkMemory::queueConfig(config, true, merged) && merged.m_onGrowth, "pool copy did not recover");
    bbCheck(receiver.openReceiver(76, 2, 0, 0, 0, 0, config), "receiver could not retry configuration");
    receiver.publish();
    ProcessWorkChannel sender;
    bbCheck(sender.openSender(bbPid()), "configuration recovery sender failed");
    std::atomic<int32_t> full(0);
    ProcessWorkReceiveQueue received;
    bbCheck(received.configure(config), "configuration recovery queue failed");
    // 每次先推进真实接收维护；只有一次发送成功时退出，失败请求不能造成额外交付。
    bbWait(
        [&]()
        {
            receiver.pump(received, full);
            return sender.trySend("123456", 6, 7) == PROCESS_WORK_SENT;
        },
        "configuration recovery send");
    char* packet = nullptr;
    // 真实搬运后借用一条记录，成功结束等待；超时由测试框架判失败。
    bbWait(
        [&]()
        {
            receiver.pump(received, full);
            return received.take(&packet, 0);
        },
        "configuration recovery receive");
    int32_t header[3] = {};
    std::memcpy(header, packet, sizeof(header));
    const bool matches =
        header[0] == bbPid() && header[1] == 7 && header[2] == 6 && std::memcmp(packet + 12, "123456", 6) == 0;
    received.release(packet);
    bbCheck(matches && !received.hasPending(), "configuration recovery changed payload or ownership");
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
    bbCheck(queue.allocate(329, result, true) == nullptr && result == LockFreeQueueInvalidConfiguration &&
            !queue.hasPending(),
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
            !receive.configure(config),
        "unpublished receive lifetime missing");
    bbCheck(receive.flush(), "unlimited receive flush");
    char* packet = nullptr;
    bbCheck(
        receive.take(&packet, 0) && receive.hasPending() && !receive.configure(config), "callback lifetime missing");
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
    bbCheck(ipc.tryPost(bbPid(), tooLarge.data(), 32, static_cast<MessageType>(1), true, true) ==
                PROCESS_WORK_POST_INVALID &&
            !ipc.post(bbPid(), tooLarge, static_cast<MessageType>(1)),
        "impossible public post did not return terminal error");
    bbCheck(ipc.addProcessReceiveCallback(&log) && ipc.initReceive(64, 2, 0, 0, 0, 0, config),
        "valid quota smaller than maximum packet incorrectly rejected configuration");
    const std::string rejected(24, 'x');
    const std::string accepted(23, 'x');
    bbCheck(
        ipc.trySend(bbPid(), rejected.data(), 24, static_cast<MessageType>(2), true, true) == PROCESS_WORK_TOO_LARGE &&
            !ipc.send(bbPid(), rejected, static_cast<MessageType>(2)),
        "remote fixed quota was not checked before admission");
    bbCheck(ipc.send(bbPid(), accepted, static_cast<MessageType>(3)), "remote exact single-message quota rejected");
    waitPackets(log, 1);
    bbCheck(log.snapshot()[0].m_type == 3 && log.snapshot()[0].m_body == accepted,
        "failed request committed or blocked next packet");
    BlackBoxChild child;
    std::vector<std::string> arguments;
    arguments.push_back("--quota-probe");
    arguments.push_back(bbNumber(bbPid()));
    child.start(arguments, bbTempPath("-quota-probe.log"));
    bbCheck(child.wait(10000) == 0, "separate-process quota probe failed");
    waitPackets(log, 2);
    bbCheck(log.snapshot()[1].m_type == 4 && log.snapshot()[1].m_body == accepted,
        "cross-process quota lost or duplicated messages");
    scope.close();
    config.m_localQueueBytes = 1;
    bbCheck(ipc.initReceive(64, 2, 0, 0, 0, 0, config), "positive tiny quota should be valid configuration");
    bbCheck(ipc.sendResult(bbPid(), nullptr, 0) == PROCESS_WORK_TOO_LARGE,
        "empty message bypassed impossible remote quota");
    scope.close();
    bbCheck(ipc.configurePostMemory(ProcessWorkMemoryConfig()), "restore default post quota");
}

void testProcessWorkMemoryConfiguration()
{
    ProcessWorkMemoryConfig config;
    bbCheck(config.m_localQueueBytes == -1 && config.m_queuePool.maximumCapacity == -1 &&
            config.m_freeQueuePool.maximumCapacity == -1,
        "default local capacity must be unlimited");
    bbCheck(config.setLocalQueueLimitMB(128) && config.m_localQueueBytes == 128LL * 1024 * 1024,
        "MB configuration did not convert to bytes");
    bbCheck(!config.setLocalQueueLimitMB(0) && !config.setLocalQueueLimitMB(-2) &&
            !config.setLocalQueueLimitMB(INT64_MAX / (1024 * 1024) + 1) &&
            config.m_localQueueBytes == 128LL * 1024 * 1024,
        "invalid MB value changed previous configuration");
    bbCheck(config.setLocalQueueLimitMB(INT64_MAX / (1024 * 1024)) &&
            config.m_localQueueBytes == (INT64_MAX / (1024 * 1024)) * (1024 * 1024),
        "largest valid MB value overflowed");
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
    bbCheck(queue.used() == 0 && queue.hasPending(), "unlimited allocations were charged or lost lifetime protection");
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
    bbCheck(!receive.append(packet.data(), packet.size()) && !receive.hasPending() &&
            policy->m_notifications == prior && policy->m_calls == priorCalls + 1,
        "real receive growth exception committed a packet or emitted success");
    WaitMemoryRetry(config.m_retryMilliseconds);
    policy->m_throwGrow = false;
    policy->m_refuse = true;
    bbCheck(!receive.append(packet.data(), packet.size()) && !receive.hasPending() &&
            policy->m_notifications == prior && policy->m_calls == priorCalls + 2,
        "real receive growth refusal changed ownership");
    WaitMemoryRetry(config.m_retryMilliseconds);
    policy->m_refuse = false;
    bbCheck(receive.append(packet.data(), packet.size()) && receive.flush() && policy->m_calls == priorCalls + 3,
        "receive dynamic append");
    char* read = nullptr;
    bbCheck(
        receive.take(&read, 0) && std::memcmp(read, packet.data(), packet.size()) == 0, "receive growth altered bytes");
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
    components.m_readThread.m_maximumTasks = 0;
    bbCheck(ipc.configure(components) && !ipc.initReceive(76, 4, 0, 0, 0, 0),
        "shared reader ignored its independent task limit");
    components.m_readThread.m_maximumTasks = -1;
    components.m_sendNames.nodes.initialCapacity = 1;
    components.m_sendNames.nodes.maximumCapacity = 0;
    bbCheck(!ipc.configure(components), "invalid name Map pool ignored");
    components.m_sendNames.nodes.maximumCapacity = -1;
    components.m_manager.m_threadMap.nodes.initialCapacity = 1;
    components.m_manager.m_threadMap.nodes.maximumCapacity = 0;
    bbCheck(ipc.configure(components), "disabled shared manager configuration was applied");
    components.m_configureManager = true;
    bbCheck(!ipc.configure(components), "enabled shared manager ignored its registry pool");
    components.m_manager.m_threadMap.nodes.maximumCapacity = -1;
    bbCheck(ipc.configure(components), "shared manager did not recover after rejected settings");
    components.m_sendChannels.nodes.initialCapacity = 1;
    components.m_sendChannels.nodes.maximumCapacity = 0;
    bbCheck(!ipc.configure(components), "invalid native Map config was silently ignored");
    components.m_sendChannels.nodes.maximumCapacity = -1;
    bbCheck(ipc.configure(components) && ipc.initReceive(76, 4, 0, 0, 0, 0) && ipc.initPostThread(),
        "component configuration recovery failed");
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
    const auto observe = [&calls, &added](size_t count, uint64_t sequence)
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
    bbCheck(
        hash.poolStats(hashStats).m_status == LockFreeHashMapOk && calls != 0 && calls == hashStats.successfulGrowCount,
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
    threads.reserve(32);
    try
    {
        for (int32_t producer = 0; producer < 32; ++producer)
        {
            threads.push_back(std::thread(
                [&queue, &start, &finished, &errors, producer]()
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
    }
    catch (...)
    {
        start = true;
        for (size_t i = 0; i < threads.size(); ++i)
        {
            threads[i].join();
        }
        throw;
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
    channel.m_sender.close();
    channel.m_receiver.stop();
    channel.m_receiver.close();
    ProcessWorkMemoryConfig config;
    std::shared_ptr<ProcessWorkTestMemoryPolicy> policy(new ProcessWorkTestMemoryPolicy);
    policy->m_increment = UINT64_MAX;
    policy->m_throwNotification = true;
    config.m_policy = policy;
    bbCheck(channel.m_receiver.openReceiver(76, 4, 0, 2, 0, 1, config), "shared policy open");
    channel.m_receiver.publish();
    bbCheck(channel.m_sender.openSender(bbPid()), "shared policy sender");
    channel.pump();
    bbCheck(policy->m_notifications == 0, "shared initial reserve was notified as growth");
    for (int32_t i = 0; i < 4; ++i)
    {
        bbCheck(channel.m_sender.trySend("x", 1, i) == PROCESS_WORK_SENT, "shared initial capacity");
    }
    bbCheck(channel.m_sender.trySend("x", 1, 4) == PROCESS_WORK_CAPACITY, "shared growth request committed twice");
    const uint64_t started = ProcessWorkTime::milliseconds();
    bbWait(
        [started]()
        {
            return ProcessWorkTime::milliseconds() - started >= 25;
        },
        "shared maintenance window");
    bbWait(
        [&channel]()
        {
            channel.pump();
            return channel.m_receiver.bufferCount() == 6;
        },
        "shared growth cap clipping");
    bbWait(
        [&channel]()
        {
            channel.pump();
            return channel.m_delivered.size() == 4;
        },
        "shared growth drain");
    bbCheck(policy->m_kindCount[ProcessWorkSharedBuffers] == 1 && policy->m_kindAdded[ProcessWorkSharedBuffers] == 2,
        "shared success notification mismatch");
    for (size_t i = 0; i < channel.m_delivered.size(); ++i)
    {
        bbCheck(channel.m_delivered[i] == static_cast<int32_t>(i), "shared growth lost/reordered data");
    }
}

void testProcessWorkDependencyOptions()
{
    uint64_t total = 0, available = 0, virtualBytes = 0;
    bbCheck(CSystem::queryMemoryAvailability(total, available, virtualBytes) && total != 0 &&
        available <= total && virtualBytes != 0, "native memory snapshot invalid");
#ifdef _WIN32
    bbCheck(virtualBytes != UINT64_MAX, "Windows virtual address budget missing");
#else
    bbCheck(virtualBytes == UINT64_MAX, "unexpected virtual address budget on POSIX");
#endif
    const uint64_t before = CSystem::monotonicMilliseconds();
    const uint64_t current = ProcessWorkTime::milliseconds();
    bbCheck(before <= current && current <= CSystem::monotonicMilliseconds(), "maintenance clock mismatch");
    bbCheck(!ProcessWork::setProcessIdentityDirectory("invalid/relative") &&
        !ProcessWork::setSharedMemoryDirectory("invalid/relative") &&
        !ProcessWork::setSemaphoreDirectory("invalid/relative"), "relative IPC directory accepted");
    ProcessWorkMemoryConfig memory;
    SemaphoreWaitPolicy wait;
    wait.m_localSlice = 7;
    bbCheck(memory.m_notification.setWaitPolicy(wait).status == SEMAPHORE_OK, "custom wait policy rejected");
    memory.m_notification.configureEventPool().initialCapacity = 3;
    memory.m_notification.configureCountPool().initialCapacity = 5;
    ProcessWorkReceiveQueue receive;
    bbCheck(receive.configure(memory) && receive.notification().waitPolicy().m_localSlice == 7 &&
        receive.notification().eventPoolStatistics().capacity == 3 &&
        receive.notification().countPoolStatistics().capacity == 5, "local wait policy or native pools ignored");
    for (int side = 0; side < 2; ++side)
    {
        for (int field = 0; field < 5; ++field)
        {
            ProcessWorkMemoryConfig invalid;
            ProcessWorkQueueOptions& options = side == 0 ? invalid.m_queueOptions : invalid.m_freeQueueOptions;
            if (field == 0) { options.m_distributedCounters = 2; }
            if (field == 1) { options.m_maintenanceInterval = 0; }
            if (field == 2) { options.m_maintenanceBudget = 0; }
            if (field == 3) { options.m_capacitySleepMilliseconds = 0; }
            if (field == 4) { options.m_consumerYieldInterval = 65537; }
            bbCheck(!receive.configure(invalid), "invalid per-queue option ignored");
        }
    }
    ProcessWorkQueueOptions& options = memory.m_queueOptions;
    bbCheck(options.configureAutomaticMaintenance(1, 2) == LockFreeQueueSuccess &&
        options.configureCapacityWait(0, 2) == LockFreeQueueSuccess &&
        options.configureConsumerYieldInterval(1) == LockFreeQueueSuccess &&
        options.configureCancelableReservations(true) == LockFreeQueueSuccess &&
        options.configureProducerCounters(true) == LockFreeQueueSuccess &&
        options.configureEntryOnly(false) == LockFreeQueueSuccess, "valid native settings rejected");
    bbCheck(options.configureAutomaticMaintenance(0, 2) == LockFreeQueueInvalidConfiguration &&
        options.configureAutomaticMaintenance(1, 0) == LockFreeQueueInvalidConfiguration &&
        options.configureCapacityWait(7, 0) == LockFreeQueueInvalidConfiguration &&
        options.configureConsumerYieldInterval(65537) == LockFreeQueueInvalidConfiguration &&
        options.configureEntryOnly(true) == LockFreeQueueInvalidConfiguration, "invalid native settings accepted");
    bbCheck(options.m_maintenanceInterval == 1 && options.m_maintenanceBudget == 2 &&
        options.m_capacityRetries == 0 && options.m_capacitySleepMilliseconds == 2 &&
        options.m_consumerYieldInterval == 1 && options.m_cancelableReservations &&
        options.m_distributedCounters == 1, "failed setter changed last valid settings");
    ProcessWorkLocalQueue post;
    bbCheck(post.configure(memory), "Post options rejected");
    uint32_t attempts = 0;
    bbCheck(post.waitCapacity(attempts) && attempts == 0, "Post ignored its zero fast-retry budget");
    memory.m_queueOptions.m_capacityRetries = 2;
    bbCheck(post.configure(memory) && post.waitCapacity(attempts) && attempts == 1 &&
        post.waitCapacity(attempts) && attempts == 2 && post.waitCapacity(attempts) && attempts == 2,
        "Post did not use configured fast-retry budget");
    bbCheck(receive.configure(memory), "valid queue options failed after rejected configurations");
    ProcessWork& ipc = ProcessWork::instance();
    PacketLog log;
    IpcScope scope;
    ProcessWorkConfig config;
    config.m_receiveMemory = memory;
    config.m_postMemory = memory;
    config.m_postMemory.m_notification.configureCountPool().maxCapacity = 1;
    bbCheck(!ipc.configure(config), "invalid Post notification ignored");
    config.m_postMemory.m_notification.configureCountPool().maxCapacity = -1;
    for (int side = 0; side < 4; ++side)
    {
        ProcessWorkConfig candidate(config);
        SemaphoreConfig* notifications[] = { &candidate.m_receiveDataReady, &candidate.m_receiveSpaceReady,
            &candidate.m_sendDataReady, &candidate.m_sendSpaceReady };
        notifications[side]->configureEventPool().maxCapacity = 0;
        bbCheck(ipc.configure(candidate), "quiescent component configuration rejected");
        if (side < 2)
        {
            bbCheck(!ipc.initReceive(64, 2), "invalid receive channel notification ignored");
        }
        else
        {
            bbCheck(ipc.initReceive(64, 2) && !ipc.prepareSend(bbPid()), "invalid sender notification ignored");
            ipc.uninitReceive();
            ipc.clear();
        }
    }
    ProcessWorkConfig senderOnly(config);
    senderOnly.m_producers.nodes.initialCapacity = 2;
    senderOnly.m_producers.nodes.maximumCapacity = 1;
    bbCheck(ipc.configure(senderOnly) && ipc.initReceive(64, 2) && !ipc.prepareSend(bbPid()),
        "sender producer pool affected receiver or was ignored by sender");
    ipc.uninitReceive();
    ipc.clear();
    bbCheck(ipc.configure(config), "valid component configuration failed after rejection");
    scope.start(&log, receiveName("settings"), 64, 2);
    sendPacket(0, bbPid(), "", "configured", 1);
    waitPackets(log, 1);
    bbCheck(log.snapshot()[0].m_body == "configured", "configured queue corrupted payload");
    scope.close();
}


void testProcessWorkMappingReserve()
{
    ProcessWorkConfig config;
    const ProcessWorkMemoryConfig defaults;
    bbCheck(config.m_sendMappingReserveBytes == defaults.m_reserveBytes &&
        config.m_sendMappingReservePercent == defaults.m_reservePercent, "unexpected sender mapping defaults");
    bbCheck(config.setSendMappingReserve(UINT64_MAX, 0) && !config.setSendMappingReserve(1, 91) &&
        config.m_sendMappingReserveBytes == UINT64_MAX && config.m_sendMappingReservePercent == 0,
        "invalid sender protection overwrote the last settings");
    ProcessWork& ipc = ProcessWork::instance();
    PacketLog ledger;
    IpcScope scope;
    config.m_sendMappingReservePercent = 91;
    bbCheck(!ipc.configure(config), "invalid raw mapping percentage accepted");
    bbCheck(config.setSendMappingReserve(UINT64_MAX, 90) && ipc.configure(config), "sender policy rejected");
    // 修改调用方副本；发送端仍须保留刚才复制的保护值，直到下次显式 configure。
    bbCheck(config.setSendMappingReserve(0, 0), "cannot edit sender configuration copy");
    scope.start(&ledger, receiveName("mapping-reserve"), 76, 2);
    bbCheck(!ipc.prepareSend(bbPid(), -1, true), "sender map ignored its own reserve");
    sendPacket(0, bbPid(), "", "ordinary", 1);
    waitPackets(ledger, 1);
    scope.close();
    bbCheck(ipc.configure(config), "sender policy reset failed");
    scope.start(&ledger, receiveName("mapping-reserve"), 76, 2);
    bbCheck(ipc.prepareSend(bbPid(), -1, true), "sender prepare failed after lowering its reserve");
    sendPacket(0, bbPid(), "", "prepared", 2);
    waitPackets(ledger, 2);
    scope.close();
    const std::vector<ReceivedPacket> packets = ledger.snapshot();
    bbCheck(packets.size() == 2 && packets[0].m_body == "ordinary" && packets[1].m_body == "prepared" &&
        packets[0].m_type == 1 && packets[1].m_type == 2, "preparation altered delivery");
}

void testProcessWorkIndependentLimits()
{
    const int32_t bytes = 1024 * 1024;
    std::string body(bytes, 'q');
    body[bytes / 2] = '\0';
    ProcessWork& ipc = ProcessWork::instance();
    for (int32_t postMB = 1; postMB <= 2; ++postMB)
    {
        ProcessWorkConfig config;
        bbCheck(config.m_postMemory.setLocalQueueLimitMB(postMB) &&
            config.m_receiveMemory.setLocalQueueLimitMB(3 - postMB) && ipc.configure(config), "set independent limits");
        // 修改调用方副本，运行中的配置仍须采用刚才交给 configure 的值。
        bbCheck(config.m_postMemory.setLocalQueueLimitMB(-1) && config.m_receiveMemory.setLocalQueueLimitMB(-1),
            "edit caller-owned settings");
        PacketLog ledger;
        IpcScope scope;
        scope.start(&ledger, receiveName("independent-limits"), bytes + 12, 2);
        const int32_t pid = bbPid();
        const bool receiveFits = postMB == 1;
        bbCheck(ipc.sendResult(pid, body.data(), bytes, static_cast<MessageType>(1)) ==
            (receiveFits ? PROCESS_WORK_SENT : PROCESS_WORK_TOO_LARGE), "receiver did not use its frozen limit");
        if (receiveFits)
        {
            waitPackets(ledger, 1);
        }
        // 后台线程尚未启动，可精确观察本地接纳是否计费。
        bbCheck(ipc.postResult(pid, body.data(), bytes, static_cast<MessageType>(2)) ==
            (receiveFits ? PROCESS_WORK_POST_INVALID : PROCESS_WORK_POSTED), "Post did not use its own frozen limit");
        bbCheck(ipc.postPendingBytes() == (receiveFits ? 0U : static_cast<uint64_t>(bytes) + 280),
            "unexpected local charge before sender startup");
        bbCheck(ipc.initPostThread() && ipc.post(pid, "end", 3, static_cast<MessageType>(3)), "Post barrier failed");
        ipc.waitForPostEnd();
        const size_t expected = receiveFits ? 2 : 1;
        waitPackets(ledger, expected);
        scope.close();
        const std::vector<ReceivedPacket> packets = ledger.snapshot();
        bbCheck(packets.size() == expected && packets.back().m_pid == pid && packets.back().m_type == 3 &&
            packets.back().m_body == "end" && (!receiveFits || (packets.front().m_pid == pid &&
            packets.front().m_type == 1 && packets.front().m_body == body)), "unexpected delivery after quota rejection");
    }
}