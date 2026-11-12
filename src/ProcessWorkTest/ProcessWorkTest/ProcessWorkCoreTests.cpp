#include "ProcessWorkCoreTests.h"
#include "ProcessWorkTest.h"
#include "ProcessWork/ProcessWorkAtomic.h"
#include "ProcessWork/ProcessWorkMemory.h"
#include "ProcessWork/ProcessWorkStorage.h"
#include "ProcessWork/ProcessWorkSharedQueue.h"
#include "ProcessWork/ProcessWorkTime.h"
#include "CSystem/CSystemAPI.h"
#include <limits>
#include <thread>

/** 以维护实际使用的单调时钟等待窗口，不把sleep时长当作维护已经到期
*/
static void WaitMaintenanceWindow()
{
    const uint64_t started = ProcessWorkTime::milliseconds();
    // 返回true时至少跨过25个维护毫秒；等待期间不消费或改变共享队列。
    bbWait([started]()
    {
        return ProcessWorkTime::milliseconds() - started >= 25;
    }, "maintenance clock window");
}

void testProcessWorkSelfClose()
{
    PacketLog log;
    IpcScope scope;
    std::atomic<bool> returned(false);
    log.action = [&returned](const ReceivedPacket&)
    {
        ProcessWork::instance().uninitReceive();
        returned.store(true);
    };
    scope.start(&log, receiveName("self-close"), 64, 4);
    sendPacket(0, bbPid(), "", "owned", 1);
    bbWait([&returned]() { return returned.load(); }, "self-close returned");
    scope.close();
    bbCheck(log.errors == 0 && log.snapshot().size() == 1 && log.snapshot()[0].body == "owned", "self-close invalidated callback data");
}

void testProcessWorkAtomics()
{
    ProcessWorkSharedDirectory counter = {};
    counter.m_nextSession = 0xffffff00LL;
    IpcJobs increments;
    for (int32_t thread = 0; thread < 4; ++thread)
    {
        // 同时跨越32位边界，确认32位构建没有丢更新或截断高位。
        increments.add([&counter]()
        {
            for (int32_t i = 0; i < 10000; ++i)
            {
                ProcessWorkAtomic::increment(&counter.m_nextSession);
            }
        });
    }
    increments.drain();
    bbCheck(ProcessWorkAtomic::load(&counter.m_nextSession) == 0xffffff00LL + 40000,
        "concurrent 64-bit increment lost updates or truncated high bits");
    int64_t compared = 0x100000007LL;
    bbCheck(ProcessWorkAtomic::compareExchange(&compared, 7, 9) == 0x100000007LL && compared == 0x100000007LL,
        "failed CAS did not return full observed value");
    bbCheck(ProcessWorkAtomic::compareExchange(&compared, 0x100000007LL, 0x200000009LL) == 0x100000007LL && compared == 0x200000009LL,
        "successful CAS did not return previous value");
    ProcessWorkSharedDirectory state = {};
    int32_t phase = 0;
    bbCheck(!ProcessWorkAtomic::replace(&phase, 7, 1) && ProcessWorkAtomic::load(&phase) == 0, "failed CAS wrote value");
    bbCheck(ProcessWorkAtomic::replace(&state.m_session, int64_t(0), INT64_MAX), "64-bit CAS failed");
    bbCheck(ProcessWorkAtomic::load(&state.m_session) == INT64_MAX, "64-bit load truncated");
    bbCheck(ProcessWorkAtomic::replace(&state.m_session, INT64_MAX, int64_t(0)), "64-bit reset failed");
    IpcJobs jobs;
    jobs.add([&state]()
    {
        for (int32_t i = 1; i <= 10000; ++i)
        {
            bbWait([&state]()
            {
                for (int32_t spin = 0; spin < 10000; ++spin)
                {
                    if (ProcessWorkAtomic::load(&state.m_session) == 0)
                    {
                        return true;
                    }
                    std::this_thread::yield();
                }
                return false;
            }, "writer phase");
            state.m_identity = static_cast<uint64_t>(i) * 0x100000001ULL;
            ProcessWorkAtomic::store(&state.m_session, static_cast<int64_t>(i) * 0x100000001LL);
        }
    });
    jobs.add([&state]()
    {
        for (int32_t i = 1; i <= 10000; ++i)
        {
            bbWait([&state]()
            {
                for (int32_t spin = 0; spin < 10000; ++spin)
                {
                    if (ProcessWorkAtomic::load(&state.m_session) != 0)
                    {
                        return true;
                    }
                    std::this_thread::yield();
                }
                return false;
            }, "reader phase");
            bbCheck(state.m_identity == static_cast<uint64_t>(i) * 0x100000001ULL, "publication tore payload");
            bbCheck(ProcessWorkAtomic::load(&state.m_session) == static_cast<int64_t>(i) * 0x100000001LL, "64-bit release store tore");
            ProcessWorkAtomic::store(&state.m_session, int64_t(0));
        }
    });
    jobs.drain();
}

void testProcessWorkMemoryGuard()
{
    ProcessWorkMemoryConfig config;
    bbCheck(ProcessWorkMemory::valid(config), "default memory config invalid");
    bbCheck(!ProcessWorkMemory::allows(config, (std::numeric_limits<uint64_t>::max)()), "overflow allocation admitted");
    config.m_reserveBytes = (std::numeric_limits<uint64_t>::max)();
    bbCheck(!ProcessWorkMemory::allows(config, 1), "reserve underflow admitted allocation");
    config.m_reserveBytes = 0;
    config.m_reservePercent = 91;
    bbCheck(!ProcessWorkMemory::valid(config) && !ProcessWorkMemory::allows(config, 0), "invalid reserve accepted");
    config.m_reservePercent = 0;
    config.m_growthBytes = 0;
    bbCheck(!ProcessWorkMemory::valid(config), "zero growth accepted");
    config.m_growthBytes = 1;
    config.m_retryMilliseconds = 0;
    bbCheck(!ProcessWorkMemory::valid(config), "zero retry interval accepted");
}

void testProcessWorkStorage()
{
    uint64_t identity = 0;
    bbCheck(CSystem::processIdentity(bbPid(), identity), "cannot query self identity");
    ProcessWorkMemoryConfig config;
    ProcessWorkStorage receiver(bbPid(), identity, 0xfffffe, 64, 1, true, config);
    ProcessWorkStorage sender(bbPid(), identity, 0xfffffe, 64, 1, false, config);
    bbCheck(receiver.create(1, 1) && receiver.create(256, 1) && receiver.create(65536, 1), "paged create failed");
    bbCheck(receiver.node(256) != nullptr && receiver.block(65536) != nullptr, "page boundary lookup failed");
    bbCheck(receiver.page(0x1000000) == nullptr && sender.page(2, true) == nullptr, "invalid directory access accepted");
    std::unique_ptr<SharedMemory> temporary;
    char* source = receiver.data(1, 1, temporary);
    bbCheck(source != nullptr, "initial body missing");
    source[0] = 'A';
    IpcJobs jobs;
    for (int32_t i = 0; i < 8; ++i)
    {
        jobs.add([&sender]()
        {
            for (int32_t n = 0; n < 1000; ++n)
            {
                std::unique_ptr<SharedMemory> view;
                char* data = sender.data(1, 1, view);
                bbCheck(data != nullptr && data[0] == 'A' && sender.node(65536) != nullptr, "concurrent mapping publication failed");
            }
        });
    }
    jobs.drain();
    ProcessWorkSharedBlock* block = receiver.block(256);
    bbCheck(ProcessWorkAtomic::replace(&block->m_owner, int64_t(0), int64_t(7)), "claim failed");
    char* old = sender.data(256, 1, temporary);
    bbCheck(old != nullptr && !receiver.retire(256) && !receiver.retire(1), "owned or initial body retired");
    old[0] = 'B';
    ProcessWorkAtomic::replace(&block->m_owner, int64_t(7), int64_t(0));
    bbCheck(receiver.retire(256) && receiver.create(256, 2), "regrowth failed");
    bbCheck(old[0] == 'B' && ProcessWorkAtomic::load(&block->m_generation) == 2, "old view/generation corrupted");
    std::unique_ptr<SharedMemory> fresh;
    char* newer = sender.data(256, 2, fresh);
    bbCheck(newer != nullptr && newer[0] == 0, "regrowth reused old payload mapping");
    bbCheck(ProcessWorkStorage::objectName('B', bbPid(), identity, 1, 0x1000000, 1).empty(), "name silently truncated index");
    bbCheck(ProcessWorkStorage::objectName('B', bbPid(), identity, 1, 65536, 1) !=
        ProcessWorkStorage::objectName('B', bbPid(), identity, 1, 0, 1), "name index alias");
}

void testProcessWorkSharedQueue()
{
    uint64_t identity = 0;
    bbCheck(CSystem::processIdentity(bbPid(), identity), "identity missing");
    ProcessWorkMemoryConfig config;
    ProcessWorkStorage storage(bbPid(), identity, 0xfffffd, 64, 2, true, config);
    ProcessWorkSharedControl control = {};
    ProcessWorkSharedQueue queue(storage, control);
    bbCheck(queue.initialize() && queue.activate(1) && queue.activate(2), "queue init failed");
    bbCheck(!queue.consume(0), "consume without front accepted");
    ProcessWorkProducerState* producer = queue.producer(2);
    bbCheck(producer != nullptr, "producer stream registration failed");
    const int64_t initialHead = ProcessWorkAtomic::load(&producer->m_shared->m_head);
    const uint32_t initialIndex = static_cast<uint32_t>(initialHead) - 1;
    const int64_t originalLink = ProcessWorkAtomic::load(&queue.node(initialIndex)->m_link);
    uint32_t slow = 0, hint = 2, next = 0;
    int32_t value = 0;
    bbCheck(queue.reserve(1, 2, 1, slow), "reserve failed");
    bbCheck(queue.enqueue(2, 22, false, 2, hint) && queue.front(next, value) && value == 22, "unpublished reservation blocked committed node");
    const uint32_t consumed = next;
    bbCheck(queue.consume(next) && !queue.front(next, value), "consume failed");
    bbCheck(!queue.consume(consumed), "same node consumed twice");
    for (int32_t i = 1; i < kProcessWorkSegmentCells; ++i)
    {
        bbCheck(queue.enqueue(2, 22 + i, false, 2, hint), "first segment fill failed");
        bbCheck(queue.front(next, value) && value == 22 + i && queue.consume(next), "first segment FIFO failed");
    }
    bbCheck(queue.enqueue(2, 11, true, 2, hint), "second segment enqueue failed");
    // 模拟本流链接提交后暂停在写游标更新之前；接收器只读取已提交链接。
    const int64_t committedTail = ProcessWorkAtomic::load(&producer->m_shared->m_writeNode);
    ProcessWorkAtomic::store(&producer->m_shared->m_writeNode, initialHead);
    bool inlined = false;
    bbCheck(queue.front(next, value, inlined) && value == 11 && inlined && queue.consume(next), "lagging producer cursor stranded stream");
    bbCheck(ProcessWorkAtomic::load(&producer->m_shared->m_head) == committedTail, "consumer did not follow committed link");
    ProcessWorkAtomic::store(&producer->m_shared->m_writeNode, committedTail);
    uint32_t reused = 0;
    bbCheck(queue.reserve(3, 2, 3, reused) && reused == initialIndex, "old stream segment not reusable");
    bbCheck(!ProcessWorkAtomic::replace(&queue.node(reused)->m_link, originalLink, originalLink | 2), "stale writer passed generation guard");
    bbCheck(!ProcessWorkAtomic::replace(&queue.node(reused)->m_cells[0], originalLink, originalLink | 99), "stale cell writer passed generation guard");
    queue.cancel(reused, 3);
    queue.cancel(slow, 1);
    bbCheck(!queue.front(next, value), "queue duplicated entry");
}

void testProcessWorkFreeSlotsWithWaiters()
{
    uint64_t identity = 0;
    bbCheck(CSystem::processIdentity(bbPid(), identity), "waiter fixture identity missing");
    ProcessWorkStorage storage(bbPid(), identity, 0xffffeb, 64, 4, true, ProcessWorkMemoryConfig());
    ProcessWorkSharedControl control = {};
    ProcessWorkSharedQueue queue(storage, control);
    bbCheck(queue.initialize() && queue.activate(1) && queue.activate(2) && queue.activate(3) && queue.activate(4),
        "waiter fixture initialization failed");
    ProcessWorkProducerState* first = queue.producer(11);
    ProcessWorkProducerState* second = queue.producer(12);
    bbCheck(first != nullptr && second != nullptr, "waiter fixture streams missing");
    queue.request(*first, false);
    queue.request(*second, false);
    uint32_t spare = 0;
    bbCheck(queue.hasWaiters() && queue.reserve(13, 4, 1, spare), "body waiter blocked unused node capacity");
    queue.cancel(spare, 13);
    ProcessWorkSharedBlock* body = storage.block(1);
    bbCheck(body != nullptr, "waiter fixture body missing");
    ProcessWorkAtomic::store(&body->m_owner, int64_t(14));
    queue.releaseBody(1, *body);
    uint32_t granted = 0;
    bbCheck(queue.takeGrant(*first, false, granted) && granted == 1 &&
        ProcessWorkAtomic::load(&body->m_owner) == 11, "first waiter did not receive returned resource directly");
    bbCheck(!ProcessWorkAtomic::replace(&body->m_owner, int64_t(0), int64_t(13)), "new request stole a granted body");
    queue.releaseBody(1, *body);
    bbCheck(queue.takeGrant(*second, false, granted) && granted == 1 &&
        ProcessWorkAtomic::load(&body->m_owner) == 12, "second waiter starved behind a new request");
    queue.releaseBody(1, *body);
    bbCheck(ProcessWorkAtomic::load(&body->m_owner) == 0, "drained waiters retained returned body");
}

void testProcessWorkNewStreamRound()
{
    uint64_t identity = 0;
    bbCheck(CSystem::processIdentity(bbPid(), identity), "identity missing");
    for (int32_t dead = 0; dead < 2; ++dead)
    {
        ProcessWorkStorage storage(bbPid(), identity, 0xfffff0 + dead, 64, 2, true, ProcessWorkMemoryConfig());
        ProcessWorkSharedControl control = {};
        ProcessWorkSharedQueue queue(storage, control);
        bbCheck(queue.initialize() && queue.activate(1) && queue.activate(2), "new-stream queue init failed");
        bbCheck(queue.producer(1) != nullptr, "old stream registration failed");
        uint32_t slot = 0;
        int32_t value = 0;
        bbCheck(!queue.front(slot, value), "old empty stream had data");
        uint32_t hint = 1;
        bbCheck(queue.enqueue(2, 123, false, 2, hint), "new stream publish failed");
        if (dead != 0)
        {
            queue.recoverToken(1);
        }
        bbCheck(queue.front(slot, value) && value == 123,
            "first front missed ready stream across old round boundary");
        bbCheck(queue.consume(slot) && !queue.front(slot, value), "new stream duplicated data");
    }
}

void testProcessWorkBorrowedCache()
{
    LockFreeMap<int32_t, std::shared_ptr<int32_t>> values;
    std::shared_ptr<int32_t> candidate(new int32_t(73));
    bbCheck(values.lockFreeMapSet(1, candidate) == LOCK_FREE_MAP_COMMITTED, "cache fixture insertion failed");
    const long references = candidate.use_count();
    int32_t called = 0;
    bbCheck(values.lockFreeMapVisit(1, [&values, &candidate, references, &called](const std::shared_ptr<int32_t>& value)
    {
        ++called;
        bbCheck(candidate.use_count() == references && value == candidate && *value == 73, "borrowed visit copied or corrupted value");
        values.clear();
        size_t released = 0;
        values.lockFreeMapMaintain(100, released);
        bbCheck(*value == 73, "reentrant erase invalidated protected value");
    }) == LOCK_FREE_MAP_OK && called == 1, "borrowed callback repeated or failed");
    bbCheck(values.lockFreeMapVisit(1, [&called](const std::shared_ptr<int32_t>&) { ++called; }) == LOCK_FREE_MAP_NOT_FOUND && called == 1,
        "missing key invoked callback");
    bbCheck(values.lockFreeMapSet(1, candidate) == LOCK_FREE_MAP_COMMITTED, "cache reinsertion failed");
    bbCheck(values.lockFreeMapVisit(1, [&called](const std::shared_ptr<int32_t>&)
    {
        ++called;
        throw std::runtime_error("intentional visit failure");
    }) == LOCK_FREE_MAP_USER_EXCEPTION && called == 2, "throwing callback retried or escaped");
}

void testProcessWorkSharedQueueStress()
{
    uint64_t identity = 0;
    bbCheck(CSystem::processIdentity(bbPid(), identity), "identity missing");
    ProcessWorkMemoryConfig config;
    ProcessWorkStorage storage(bbPid(), identity, 0xfffffc, 64, 256, true, config);
    ProcessWorkSharedControl control = {};
    ProcessWorkSharedQueue queue(storage, control);
    bbCheck(queue.initialize(), "queue init failed");
    for (uint32_t i = 1; i <= 256; ++i)
    {
        bbCheck(queue.activate(i), "activate failed");
    }
    const int32_t perProducer = 50000;
    std::atomic<bool> cancel(false);
    IpcJobs jobs;
    for (int32_t producer = 0; producer < 8; ++producer)
    {
        jobs.add([&queue, &cancel, producer, perProducer]()
        {
            uint32_t hint = static_cast<uint32_t>(producer);
            for (int32_t sequence = 0; sequence < perProducer && !cancel.load(); ++sequence)
            {
                while (!cancel.load() && !queue.enqueue(producer + 1, producer * perProducer + sequence + 1, false, 256, hint))
                {
                    std::this_thread::yield();
                }
            }
        });
    }
    int32_t expected[8] = {};
    const uint64_t deadline = bbNow() + 30000;
    try
    {
        for (int32_t received = 0; received < perProducer * 8;)
        {
            uint32_t index = 0;
            int32_t value = 0;
            if (!queue.front(index, value))
            {
                bbCheck(bbNow() < deadline, "queue stress deadline");
                std::this_thread::yield();
                continue;
            }
            --value;
            const int32_t producer = value / perProducer;
            bbCheck(value >= 0 && producer < 8 && value % perProducer == expected[producer], "queue lost, duplicated or reordered item");
            ++expected[producer];
            bbCheck(queue.consume(index), "queue consume failed");
            ++received;
        }
        jobs.drain();
    }
    catch (...)
    {
        cancel.store(true);
        throw;
    }
}

void testProcessWorkSenderAddressCache()
{
    PublicChannelScope channel(1024, 0, 0, 0, 0);
    ProcessWorkChannel alternate;
    bbCheck(alternate.openSender(bbPid()), "alternate sender open failed");
    int32_t sequence = 0;
    for (int32_t round = 0; round < 40; ++round)
    {
        for (int32_t item = 0; item < 700; ++item)
        {
            ProcessWorkChannel& sender = (item & 1) == 0 ? channel.sender : alternate;
            // 仅尚未登记时通过公共维护推进，成功一次即停止重试，不重复投递。
            bbWait([&sender, &channel, sequence]()
            {
                const int32_t result = sender.trySend("x", 1, sequence);
                if (result == PROCESS_WORK_SENT)
                {
                    return true;
                }
                bbCheck(result == PROCESS_WORK_RESOURCE || result == PROCESS_WORK_CAPACITY,
                    "alternate sender returned unexpected failure");
                channel.pump();
                return false;
            }, "alternating sender admission");
            ++sequence;
        }
        bbWait([&channel, sequence]()
        {
            channel.pump();
            return channel.delivered.size() == static_cast<size_t>(sequence);
        }, "alternating sender drain");
        if (round % 7 == 6)
        {
            alternate.close();
            bbCheck(alternate.openSender(bbPid()), "alternate sender reopen failed");
        }
    }
    for (int32_t index = 0; index < sequence; ++index)
    {
        bbCheck(channel.delivered[index] == index, "cached sender address lost or reordered message");
    }
}

void testProcessWorkReservationWindow()
{
    const int32_t capacities[] = { 1, 63, 64, 65, 129, 255, 256, 257, 511, 512, 513 };
    for (size_t sample = 0; sample < sizeof(capacities) / sizeof(capacities[0]); ++sample)
    {
        PublicChannelScope channel(capacities[sample], 0, 0, 0, 0);
        int32_t sequence = 0;
        for (int32_t round = 0; round < 4; ++round)
        {
            for (int32_t item = 0; item < capacities[sample]; ++item)
            {
                bbCheck(channel.sender.trySend("x", 1, sequence) == PROCESS_WORK_SENT,
                    "reservation hint hid available shared capacity");
                ++sequence;
            }
            bbCheck(channel.sender.trySend("x", 1, sequence) == PROCESS_WORK_CAPACITY,
                "full shared capacity was bypassed or misclassified as contention");
            // 仅消费至本轮最后序号；失败的额外发送不能留下重复提交。
            bbWait([&channel, sequence]()
            {
                channel.pump();
                return channel.delivered.size() == static_cast<size_t>(sequence);
            }, "reservation window drain");
        }
        bbCheck(channel.receiver.bufferCount() == capacities[sample], "fixed window test grew shared capacity");
        for (int32_t item = 0; item < sequence; ++item)
        {
            bbCheck(channel.delivered[item] == item, "reservation window reuse lost FIFO");
        }
    }
}

void testProcessWorkUnlimited()
{
    PublicChannelScope channel(1024, 0, 128, 0, -1);
    int32_t sequence = 0;
    for (int32_t i = 0; i < 1024; ++i)
    {
        bbCheck(channel.sender.trySend("x", 1, sequence++) == PROCESS_WORK_SENT, "initial capacity not available");
    }
    WaitMaintenanceWindow();
    channel.pump();
    bbCheck(channel.receiver.bufferCount() == 1024, "threshold zero grew before an unsatisfied request");
    for (int32_t i = 0; i < 256; ++i)
    {
        bbCheck(channel.sender.trySend("x", 1, sequence++) == PROCESS_WORK_SENT, "refill failed");
    }
    bbCheck(channel.sender.trySend("x", 1, sequence) == PROCESS_WORK_CAPACITY, "full request unexpectedly committed");
    WaitMaintenanceWindow();
    bbWait([&channel]() { channel.pump(); return channel.receiver.bufferCount() == 1152; }, "unlimited expansion crossed 1024");
    bbWait([&channel, sequence]() { channel.pump(); return channel.delivered.size() == static_cast<size_t>(sequence); }, "unlimited drain");
    for (int32_t i = 0; i < sequence; ++i)
    {
        bbCheck(channel.delivered[i] == i, "unlimited growth lost order");
    }
    // 再产生一个完整扩容请求，证明-1没有被当作单次扩容或固定旧容量。
    for (int32_t i = 0; i < 1152; ++i)
    {
        bbCheck(channel.sender.trySend("x", 1, sequence++) == PROCESS_WORK_SENT, "expanded capacity unavailable");
    }
    bbCheck(channel.sender.trySend("x", 1, sequence) == PROCESS_WORK_CAPACITY, "second full request committed");
    WaitMaintenanceWindow();
    bbWait([&channel]() { channel.pump(); return channel.receiver.bufferCount() == 1280; }, "second unlimited expansion");
    bbWait([&channel, sequence]() { channel.pump(); return channel.delivered.size() == static_cast<size_t>(sequence); }, "second drain");
    for (int32_t i = 0; i < sequence; ++i)
    {
        bbCheck(channel.delivered[i] == i, "second expansion ledger mismatch");
    }
}

void testProcessWorkReceiverPageReuse()
{
    const int32_t capacities[] = { 513, 257, 3 };
    PublicChannelScope channel(capacities[0], 0, 0, 0, 0);
    int32_t sequence = 0;
    for (size_t cycle = 0; cycle < 3; ++cycle)
    {
        if (cycle != 0)
        {
            channel.sender.close();
            channel.receiver.close();
            bbCheck(channel.receiver.openReceiver(76, capacities[cycle], 0, 0, 0, 0), "receiver reopen failed");
            channel.receiver.publish();
            bbCheck(channel.sender.openSender(bbPid()), "sender reconnect failed");
            channel.pump();
        }
        for (int32_t batch = 0; batch < 4; ++batch)
        {
            for (int32_t item = 0; item < capacities[cycle]; ++item)
            {
                // 尚未登记时由公共维护推进；一旦成功即结束，不能重放已提交消息。
                bbWait([&channel, sequence]()
                {
                    const int32_t result = channel.sender.trySend("x", 1, sequence);
                    if (result == PROCESS_WORK_SENT)
                    {
                        return true;
                    }
                    bbCheck(result == PROCESS_WORK_RESOURCE || result == PROCESS_WORK_CAPACITY,
                        "receiver reuse unexpected send status");
                    channel.pump();
                    return false;
                }, "receiver page reuse send");
                ++sequence;
            }
            // 返回true时全部消息已消费，随后才复用槽位或关闭旧映射。
            bbWait([&channel, sequence]()
            {
                channel.pump();
                return channel.delivered.size() == static_cast<size_t>(sequence);
            }, "receiver page reuse drain");
        }
    }
    bbCheck(sequence == 3092, "receiver reuse fixture count changed");
    for (int32_t item = 0; item < sequence; ++item)
    {
        bbCheck(channel.delivered[item] == item, "receiver page reuse lost FIFO or used stale mapping");
    }
}

void testProcessWorkGrowthBudget()
{
    ProcessWorkMemoryConfig config;
    config.m_reserveBytes = (std::numeric_limits<uint64_t>::max)();
    ProcessWorkChannel receiver, sender;
    bbCheck(!receiver.openReceiver(64, 1, 0, 4, 0, -1, config), "memory guard allowed impossible reserve");
    bbCheck(receiver.bufferCount() == 0 && !receiver.ready(), "rejected configuration leaked receiver");
    config = ProcessWorkMemoryConfig();
    config.m_growthBytes = 1;
    bbCheck(receiver.openReceiver(64, 1, 0, 4, 0, 1, config), "retry after pressure rejection failed");
    receiver.publish();
    bbCheck(sender.openSender(bbPid()), "sender open failed");
    LockFreeQueue<char*> queue;
    Semaphore notification;
    std::atomic<int32_t> full(0);
    receiver.pump(queue, notification, full);
    bbCheck(sender.trySend("x", 1, 0) == PROCESS_WORK_SENT && sender.trySend("x", 1, 1) == PROCESS_WORK_CAPACITY, "budget setup failed");
    WaitMaintenanceWindow();
    receiver.pump(queue, notification, full);
    bbCheck(receiver.bufferCount() == 2, "single maintenance exceeded one-block budget");
    bbWait([&receiver, &queue, &notification, &full]()
    {
        receiver.pump(queue, notification, full);
        return receiver.bufferCount() == 5;
    }, "staged growth completes planned increment");
    char* packet = nullptr;
    bbCheck(queue.pop(&packet) && packet != nullptr, "staged growth lost packet");
    delete[] packet;
    bbCheck(!queue.pop(&packet), "BUSY committed extra packet");
}