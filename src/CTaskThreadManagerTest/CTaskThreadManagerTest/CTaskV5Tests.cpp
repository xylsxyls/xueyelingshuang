#include "CTaskV5Tests.h"
#include <algorithm>
#include <chrono>
#include <climits>
#include <iostream>
#include <thread>
#include <iomanip>
#ifndef CTTM_TEST_DLL
#include "CTaskThreadManager/CTaskStorage.h"
#include "CTaskPrivateTestAccess.h"
#include "CTaskV3ModuleTests.h"
#endif

CTaskV5ChannelRecord::CTaskV5ChannelRecord() :
m_hazard(nullptr),
m_generation(0),
m_hint(0),
m_retries(0)
{

}

CTaskV5ChannelPool::~CTaskV5ChannelPool()
{
    for (size_t i = 0; i < m_segments.size(); ++i)
    {
        delete m_segments[i];
    }
}

LockFreeQueueChannelSegment<int>* CTaskV5ChannelPool::acquire()
{
    std::unique_ptr<LockFreeQueueChannelSegment<int>> segment(new LockFreeQueueChannelSegment<int>());
    segment->m_generation = m_segments.size() + 1;
    m_segments.push_back(segment.get());
    return segment.release();
}

LockFreeQueueChannelSegment<int>* CTaskV5ChannelPool::acquireInitial()
{
    return acquire();
}

void CTaskV5ChannelPool::release(LockFreeQueueChannelSegment<int>* segment)
{
    (void)segment;
}

void CTaskV5ChannelPool::retire(LockFreeQueueChannelSegment<int>* segment)
{
    (void)segment;
}

CTaskV5HistoryItem::CTaskV5HistoryItem() :
m_begin(0),
m_end(0),
m_position(static_cast<size_t>(-1))
{

}

CTaskV5EmptyObservation::CTaskV5EmptyObservation(uint64_t begin, uint64_t end, size_t delivered) :
m_begin(begin),
m_end(end),
m_delivered(delivered)
{

}

/** 从调用方时间区间验证跨生产者的非重叠 FIFO 和禁止虚假空观察，不依赖内部槽位顺序
@param [in] reservations 是否使用新预约方式；原方式使用相同判定
@param [in] entryAddresses 是否通过 Entry 地址接口消费
*/
static void RunQueueHistoryTest(bool reservations, bool entryAddresses = false)
{
    for (size_t round = 0; round < 8; ++round)
    {
        // 异常退出时先析构队列，外部 Entry 仍保持有效。
        LockFreeQueueEntry<size_t> entries[256];
        LockFreeQueue<size_t> queue(false);
        LockFreeQueuePoolConfig config;
        config.initialCapacity = 1025;
        config.maximumCapacity = 1025;
        config.maximumProducers = 4;
        bbCheck(queue.configurePool(config) == LockFreeQueueSuccess &&
            queue.configureCancelableReservations(reservations) == LockFreeQueueSuccess &&
            queue.configureEntryOnly(entryAddresses) == LockFreeQueueSuccess &&
            queue.tryInit() == LockFreeQueueSuccess, "history queue initialization");
        CTaskV5HistoryItem items[256];
        std::vector<CTaskV5EmptyObservation> empty;
        std::vector<std::thread> producers;
        producers.reserve(4);
        std::atomic<uint64_t> clock(1);
        std::atomic<bool> start(false);
        std::atomic<bool> stop(false);
        std::atomic<bool> failed(false);
        std::atomic<int32_t> failureStatus(-1);
        size_t delivered = 0;
        try
        {
            for (size_t producer = 0; producer < 4; ++producer)
            {
                // 每个生产者独占 64 条调用记录；事件序号只排序外部调用区间。
                producers.push_back(std::thread([&, producer]()
                {
                    try
                    {
                        while (!start.load() && !stop.load())
                        {
                            std::this_thread::yield();
                        }
                        for (size_t i = 0; i < 64 && !stop.load() && !failed.load();)
                        {
                            const size_t value = producer * 64 + i;
                            items[value].m_begin = clock.fetch_add(1);
                            if ((value & 7) == 0)
                            {
                                std::this_thread::yield();
                            }
                            entries[value].m_value = value;
                            const LockFreeQueueResult result = entryAddresses ? queue.tryPushEntry(entries[value]) : queue.tryPush(value);
                            if (result == LockFreeQueueTemporaryUnavailable || result == LockFreeQueueResourceUnavailable ||
                                result == LockFreeQueueCapacityLimit)
                            {
                                // 有界登记也可能被维护短暂借用；这几类结果均未提交。
                                // 保留原任务重试，成功区间从下一次调用重新记录；外层限时检查进展。
                                std::this_thread::yield();
                                continue;
                            }
                            if (result != LockFreeQueueSuccess)
                            {
                                failureStatus.store(static_cast<int32_t>(result));
                                failed.store(true);
                                break;
                            }
                            items[value].m_end = clock.fetch_add(1);
                            ++i;
                        }
                    }
                    catch (...)
                    {
                        failed.store(true);
                    }
                }));
            }
            start.store(true);
            const std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() + std::chrono::seconds(20);
            while (delivered != 256 && !failed.load())
            {
                size_t value = 256;
                const uint64_t begin = clock.fetch_add(1);
                bool found = false;
                if (entryAddresses)
                {
                    LockFreeQueueEntry<size_t>* entry = nullptr;
                    size_t received = 0;
                    found = queue.popReadyEntryPrefix(&entry, 1, received);
                    if (found)
                    {
                        bbCheck(received == 1 && entry != nullptr, "entry history invalid result");
                        value = entry->m_value;
                    }
                }
                else
                {
                    found = queue.popReady(&value);
                }
                const uint64_t end = clock.fetch_add(1);
                if (found)
                {
                    bbCheck(value < 256 && items[value].m_position == static_cast<size_t>(-1),
                        "history contained unknown or duplicate payload");
                    items[value].m_position = delivered++;
                }
                else
                {
                    empty.push_back(CTaskV5EmptyObservation(begin, end, delivered));
                    queue.maintain(8);
                    std::this_thread::yield();
                }
                bbCheck(std::chrono::steady_clock::now() < deadline, "history queue progress timeout");
            }
        }
        catch (...)
        {
            stop.store(true);
            start.store(true);
            for (size_t i = 0; i < producers.size(); ++i)
            {
                producers[i].join();
            }
            throw;
        }
        for (size_t i = 0; i < producers.size(); ++i)
        {
            producers[i].join();
        }
        if (failed.load())
        {
            std::cerr << "history reservations=" << reservations << " round=" << round << " status=" << failureStatus.load()
                << " delivered=" << delivered << '\n';
        }
        bbCheck(!failed.load() && delivered == 256 && queue.size() == 0, "history total or producer failure");
        for (size_t a = 0; a < 256; ++a)
        {
            bbCheck(items[a].m_begin != 0 && items[a].m_end > items[a].m_begin && items[a].m_position < 256,
                "history missing a successful interval");
            for (size_t b = 0; b < 256; ++b)
            {
                bbCheck(items[a].m_end >= items[b].m_begin || items[a].m_position < items[b].m_position,
                    "completed enqueue was overtaken by a later non-overlapping enqueue");
            }
            for (size_t observation = 0; observation < empty.size(); ++observation)
            {
                bbCheck(items[a].m_end >= empty[observation].m_begin || items[a].m_position < empty[observation].m_delivered,
                    "empty observation bypassed a previously completed enqueue");
            }
        }
    }
}

/** 验证 Entry 地址前缀的固定池、扩容池、双发布模式及额度归还
*/
static void RunEntryAddressPrefixTest()
{
    LockFreeQueue<size_t> ordinary;
    bbCheck(ordinary.tryPush(7) == LockFreeQueueSuccess, "entry address ordinary setup");
    LockFreeQueueEntry<size_t>* address[64] = {};
    size_t received = 99;
    bbCheck(!ordinary.popReadyEntryPrefix(address, 64, received) && received == 0,
        "entry address accepted ordinary mode");
    size_t value = 0;
    bbCheck(ordinary.pop(&value) && value == 7, "entry address moved ordinary head");
    for (size_t mode = 0; mode < 2; ++mode)
    {
        // 同一对象交替固定/可扩容配置，检查换代后不沿用旧额度块。
        // 出错时队列必须先释放借用，随后才析构外部载荷。
        std::vector<LockFreeQueueEntry<size_t>> entries(1025);
        LockFreeQueue<size_t> queue(false);
        for (size_t generation = 0; generation < 4; ++generation)
        {
            queue.destroy();
            LockFreeQueuePoolConfig config;
            config.initialCapacity = (generation & 1) != 0 ? 17 : 1025;
            config.maximumCapacity = 1025;
            config.maximumProducers = 4;
            config.growThreshold = 0;
            bbCheck(queue.configurePool(config) == LockFreeQueueSuccess &&
                queue.configureEntryOnly(true) == LockFreeQueueSuccess &&
                queue.configureCancelableReservations(mode != 0) == LockFreeQueueSuccess &&
                queue.tryInit() == LockFreeQueueSuccess, "entry address configuration");
            for (size_t round = 0; round < 3; ++round)
            {
                for (size_t i = 0; i < 1024; ++i)
                {
                    entries[i].m_value = i + round * 1024;
                    bbCheck(queue.tryPushEntry(entries[i]) == LockFreeQueueSuccess, "entry address quota refill");
                }
                bbCheck(queue.size() == 1024 && queue.tryPushEntry(entries[1024]) == LockFreeQueueCapacityLimit,
                    "entry address exact finite capacity");
                received = 99;
                bbCheck(!queue.popReadyEntryPrefix(address, 0, received) && received == 0 && queue.size() == 1024,
                    "entry address zero budget consumed data");
                size_t taken = 0;
                const size_t budgets[] = {1, 3, 64, SIZE_MAX};
                while (taken != 1024)
                {
                    const size_t budget = budgets[taken % 4];
                    bbCheck(queue.popReadyEntryPrefix(address, budget, received) && received != 0 &&
                        received <= (std::min)(budget, size_t(64)), "entry address prefix budget");
                    for (size_t i = 0; i < received; ++i)
                    {
                        bbCheck(address[i] == &entries[taken + i] && address[i]->m_value == taken + i + round * 1024,
                            "entry address FIFO or identity");
                    }
                    taken += received;
                    if (taken != 1024 && taken % 3 == 0)
                    {
                        bbCheck(queue.pop(&value) && value == taken + round * 1024, "entry address mixed pop order");
                        ++taken;
                    }
                    bbCheck(queue.size() == 1024 - taken, "entry address prefix consumption count drifted");
                }
                bbCheck(queue.empty() && !queue.popReadyEntryPrefix(address, 64, received) && received == 0,
                    "entry address retained consumed items");
                bbCheck(queue.tryPushEntry(entries[0]) == LockFreeQueueSuccess &&
                    queue.popReadyEntryPrefix(nullptr, 64, received) && received == 1 && queue.empty(),
                    "entry address discard or reusable credit");
                // 跨越短前缀降级预算后仍可丢弃整批，不能解引用空输出或遗漏额度。
                for (size_t i = 0; i < 128; ++i)
                {
                    bbCheck(queue.tryPushEntry(entries[i]) == LockFreeQueueSuccess, "entry address discard refill");
                }
                size_t discarded = 0;
                while (discarded != 128)
                {
                    bbCheck(queue.popReadyEntryPrefix(nullptr, 64, received) && received != 0 && received <= 64 &&
                        received <= 128 - discarded, "entry address batch discard count");
                    discarded += received;
                }
                bbCheck(queue.empty(), "entry address batch discard retained data");
                // 空队列仍可有闲置生产者预留；公开维护归还缓存后才应只占哨兵额度。
                bbCheck(queue.maintain(64).result == LockFreeQueueSuccess, "entry address idle cache maintenance failed");
                const LockFreeQueuePoolStatistics snapshot = queue.poolStatistics();
                if (snapshot.usedNodes != 1 || snapshot.reusableFreeCapacity + 1 != snapshot.currentCapacity)
                {
                    std::cout << "ENTRY_CREDITS mode=" << mode << " generation=" << generation << " round=" << round <<
                        " capacity=" << snapshot.currentCapacity << " used=" << snapshot.usedNodes <<
                        " free=" << snapshot.reusableFreeCapacity << " producers=" << snapshot.activeProducers << std::endl;
                }
                bbCheck(snapshot.usedNodes == 1 && snapshot.reusableFreeCapacity + 1 == snapshot.currentCapacity,
                    "entry address consumed credits were missing from pool statistics");
            }
            queue.destroy();
            bbCheck(queue.tryInit() == LockFreeQueueSuccess && queue.empty(), "entry address generation reset");
        }
    }
#ifndef CTTM_TEST_DLL
    LockFreeQueueEntry<CTaskV4MoveAssignmentValue> item(CTaskV4MoveAssignmentValue(23));
    LockFreeQueue<CTaskV4MoveAssignmentValue> noCopy(false);
    bbCheck(noCopy.configureEntryOnly(true) == LockFreeQueueSuccess && noCopy.tryInit() == LockFreeQueueSuccess &&
        noCopy.tryPushEntry(item) == LockFreeQueueSuccess, "entry address copy-free setup");
    CTaskV4MoveAssignmentValue::s_failCopy = true;
    try
    {
        LockFreeQueueEntry<CTaskV4MoveAssignmentValue>* output = nullptr;
        bbCheck(noCopy.popReadyEntryPrefix(&output, 1, received) && received == 1 && output == &item &&
            item.m_value.m_value == 23, "entry address copied or moved payload");
    }
    catch (...)
    {
        CTaskV4MoveAssignmentValue::s_failCopy = false;
        throw;
    }
    CTaskV4MoveAssignmentValue::s_failCopy = false;
#endif
}

void taskV5ChannelPublicationTest()
{
    RunEntryAddressPrefixTest();
    RunQueueHistoryTest(false);
    RunQueueHistoryTest(true);
    RunQueueHistoryTest(false, true);
    RunQueueHistoryTest(true, true);
    for (size_t mode = 0; mode < 2; ++mode)
    {
        CTaskV5ChannelPool prefixPool;
        CTaskV5ChannelRecord prefixRecord;
        LockFreeQueueChannel<int, CTaskV5ChannelPool, CTaskV5ChannelRecord> prefix(prefixPool.acquire());
        prefix.reset(prefix.consumerSegment(), mode != 0);
        int values[320] = {};
        int* output[64] = {};
        for (size_t wave = 0; wave < 5; ++wave)
        {
            const size_t base = wave * 64;
            bbCheck(prefix.publish(&values[base], prefixRecord, prefixPool) &&
                prefix.front(prefixPool) == &values[base], "prefix first publication was not visible");
            // 首次观察只有一项；后续发布再被同一个前缀消费，覆盖越过旧观察范围和跨段。
            for (size_t i = 1; i < 64; ++i)
            {
                bbCheck(prefix.publish(&values[base + i], prefixRecord, prefixPool), "prefix later publication failed");
            }
            bbCheck(prefix.readyPrefix(output, 64) == 64, "prefix omitted later completed publications");
            for (size_t i = 0; i < 64; ++i)
            {
                bbCheck(output[i] == &values[base + i], "prefix reordered or duplicated a publication");
            }
            prefix.consume(64);
            bbCheck(prefix.front(prefixPool) == nullptr, "prefix left a phantom item after complete consumption");
        }
        prefix.reset(prefixPool.acquire(), mode != 0);
        bbCheck(prefix.front(prefixPool) == nullptr && prefix.publish(&values[0], prefixRecord, prefixPool) &&
            prefix.front(prefixPool) == &values[0], "quiescent reset retained a previous generation observation");
        prefix.consume();
        bbCheck(prefix.front(prefixPool) == nullptr, "reset generation duplicated its only item");
    }
    {
        CTaskV5ChannelPool reservedPool;
        CTaskV5ChannelRecord reservedRecord;
        LockFreeQueueChannelSegment<int>* firstReserved = reservedPool.acquire();
        LockFreeQueueChannel<int, CTaskV5ChannelPool, CTaskV5ChannelRecord> reserved(firstReserved);
        reserved.reset(firstReserved, true);
        int early = 1;
        int later = 2;
        // 串行模型精确停在预约之后；后继调用真实 publish，消费必须能取消空洞而继续。
        const size_t held = firstReserved->m_tail.fetch_add(1);
        bbCheck(held == 0 && reserved.front(reservedPool) == nullptr && firstReserved->m_cells[0].load() == 0,
            "reserved empty observation unnecessarily cancelled an in-flight producer");
        bbCheck(reserved.publish(&later, reservedRecord, reservedPool) && reserved.possiblyReady(),
            "reserved successor was not visible past an unfilled head");
        bbCheck(reserved.front(reservedPool) == &later, "reserved head blocked the completed successor");
        reserved.consume();
        uintptr_t expected = 0;
        bbCheck(!firstReserved->m_cells[0].compare_exchange_strong(expected, reinterpret_cast<uintptr_t>(&early)) && expected == 1,
            "cancelled reservation accepted a delayed payload");
        bbCheck(reserved.publish(&early, reservedRecord, reservedPool) && reserved.front(reservedPool) == &early,
            "cancelled producer could not retry behind the delivered successor");
        reserved.consume();
        bbCheck(reserved.front(reservedPool) == nullptr, "reserved retry duplicated a payload");

        // 另一种合法交错：低序预约在消费前填完，必须先于后继；两次投递互相重叠。
        const size_t overlapping = firstReserved->m_tail.fetch_add(1);
        bbCheck(reserved.publish(&later, reservedRecord, reservedPool), "overlapping successor publication");
        expected = 0;
        bbCheck(firstReserved->m_cells[LockFreeQueueChannelSegment<int>::mapped(overlapping)].compare_exchange_strong(
            expected, reinterpret_cast<uintptr_t>(&early)), "overlapping earlier reservation was not available");
        bbCheck(reserved.front(reservedPool) == &early, "overlapping valid reservation order changed");
        reserved.consume();
        bbCheck(reserved.front(reservedPool) == &later, "overlapping successor lost");
        reserved.consume();
        bbCheck(reserved.front(reservedPool) == nullptr && reserved.seal(reservedPool), "reserved partial seal");
        bbCheck(reserved.publish(&later, reservedRecord, reservedPool) && reserved.front(reservedPool) == &later,
            "reserved publication after partial seal");
        reserved.consume();
        bbCheck(reserved.front(reservedPool) == nullptr, "reserved seal duplicated work");
        for (size_t round = 0; round < 8; ++round)
        {
            bbCheck(!reserved.possiblyReady() && reserved.front(reservedPool) == nullptr,
                "empty active segment reported a published item");
            bbCheck(reserved.seal(reservedPool) && reserved.possiblyReady(),
                "partial seal was hidden by the cached empty observation");
            bbCheck(reserved.publish(&later, reservedRecord, reservedPool) && reserved.possiblyReady() &&
                reserved.front(reservedPool) == &later,
                "completed post after partial seal was hidden by an empty head");
            reserved.consume();
            bbCheck(reserved.front(reservedPool) == nullptr, "repeated partial seal duplicated work");
        }
    }
    CTaskV5ChannelPool pool;
    CTaskV5ChannelRecord record;
    LockFreeQueueChannelSegment<int>* first = pool.acquire();
    LockFreeQueueChannel<int, CTaskV5ChannelPool, CTaskV5ChannelRecord> channel(first);
    int late = 1;
    int ready = 2;
    // 私有准备不修改共享发布位置；真正的暂停交错由 Queue 原有用例 16、17、29 验证。
    bbCheck(channel.front(pool) == nullptr && first->m_cells[0].load() == 0,
        "V5 empty observation changed the first publication cell");
    bbCheck(channel.publish(&ready, record, pool), "V5 successor publication");
    bbCheck(channel.publish(&late, record, pool), "V5 later publication");
    bbCheck(channel.front(pool) == &ready, "V5 later publisher overtook committed work");
    channel.consume();
    bbCheck(channel.front(pool) == &late, "V5 later publisher FIFO");
    channel.consume();
    bbCheck(channel.front(pool) == nullptr, "V5 duplicate after ordered publication");
    // 连续真实发布跨越段边界，不能用改写共享尾下标制造未发布的空洞。
    for (size_t i = 2; i <= LockFreeQueueChannelSegment<int>::kCellCount; ++i)
    {
        bbCheck(channel.publish(&ready, record, pool), "V5 cross-segment publication");
    }
    for (size_t i = 2; i <= LockFreeQueueChannelSegment<int>::kCellCount; ++i)
    {
        bbCheck(channel.front(pool) == &ready, "V5 cross-segment FIFO");
        channel.consume();
    }
    bbCheck(channel.front(pool) == nullptr && channel.seal(pool),
        "V5 partial seal contract");
    bbCheck(channel.publish(&late, record, pool) && channel.front(pool) == &late, "V5 post after partial seal");
    channel.consume();
    bbCheck(channel.front(pool) == nullptr, "V5 final channel empty");
    LockFreeQueueChannelSegment<int>* sealed = channel.consumerSegment();
    bbCheck(channel.seal(pool) && channel.front(pool) == nullptr && channel.consumerSegment() != sealed,
        "V5 empty observation retained a closed segment instead of allowing shrink");

    LockFreeQueue<size_t> quota(false);
    LockFreeQueuePoolConfig config;
    config.initialCapacity = 4096;
    config.maximumCapacity = 4096;
    config.maximumProducers = 32;
    // 基础池的一项物理额度用于保留哨兵，与现有容量契约一致。
    const size_t usable = config.initialCapacity - 1;
    bbCheck(quota.configurePool(config) == LockFreeQueueSuccess && quota.tryInit() == LockFreeQueueSuccess,
        "V5 many-producer quota initialization");
    LockFreeQueueProducer<size_t> producers[32];
    for (size_t i = 0; i < 32; ++i)
    {
        bbCheck(quota.tryAcquireProducer(producers[i]) == LockFreeQueueSuccess, "V5 producer registration");
    }
    bbCheck(producers[31].tryPush(1) == LockFreeQueueSuccess, "V5 last producer initial item");
    for (size_t i = 0; i < 31; ++i)
    {
        producers[i].release();
    }
    size_t accepted = 1;
    LockFreeQueueResult result = LockFreeQueueSuccess;
    while (accepted < usable && (result = quota.tryPush(accepted + 1)) == LockFreeQueueSuccess)
    {
        ++accepted;
    }
    // 活跃上下文的私有缓存也占硬额度；新版将这类资源不足明确返回 CapacityLimit。
    // 下面仍必须验证释放上下文后全部额度恢复，不能靠改状态检查掩盖额度泄漏。
    bbCheck(accepted < usable && result == LockFreeQueueCapacityLimit,
        "V5 pinned producer cache did not report the documented capacity limit");
    producers[31].release();
    while (accepted < usable)
    {
        result = quota.tryPush(accepted + 1);
        if (result != LockFreeQueueSuccess)
        {
            std::cerr << "last_producer_quota accepted=" << accepted << " status=" << result << '\n';
        }
        bbCheck(result == LockFreeQueueSuccess, "V5 released last-producer quota was lost");
        ++accepted;
    }
    bbCheck(quota.tryPush(usable + 1) == LockFreeQueueCapacityLimit, "V5 physical quota exceeded");
    for (size_t i = 1; i <= usable; ++i)
    {
        size_t value = 0;
        bbCheck(quota.pop(&value) && value == i, "V5 cached quota changed committed FIFO");
    }
    bbCheck(quota.empty(), "V5 cached quota retained extra items");

    LockFreeQueue<int*> pointers(false);
    LockFreeQueuePoolConfig pointerConfig;
    pointerConfig.initialCapacity = 1025;
    pointerConfig.maximumCapacity = 1025;
    bbCheck(pointers.configurePool(pointerConfig) == LockFreeQueueSuccess &&
        pointers.configureEntryOnly(true) == LockFreeQueueSuccess && pointers.tryInit() == LockFreeQueueSuccess,
        "V5 direct pointer initialization failed");
    LockFreeQueueEntry<int*> entries[1024];
    int values[1024] = {};
    for (size_t round = 0; round < 3; ++round)
    {
        for (size_t i = 0; i < 1024; ++i)
        {
            entries[i].m_value = (i + round) % 7 == 0 ? nullptr : &values[(i + round) % 1024];
            bbCheck(pointers.tryPushEntry(entries[i]) == LockFreeQueueSuccess,
                "V5 direct pointer publication lost reusable capacity");
        }
        for (size_t consumed = 0; consumed < 1024;)
        {
            int* output[64] = {};
            size_t received = 0;
            bbCheck(pointers.popReadyPrefix(output, 64, received) && received != 0,
                "V5 direct pointer prefix reported an empty nonempty queue");
            for (size_t i = 0; i < received; ++i)
            {
                bbCheck(output[i] == entries[consumed + i].m_value,
                    "V5 direct pointer changed FIFO, null value, or reused entry identity");
            }
            consumed += received;
        }
        bbCheck(pointers.empty(), "V5 direct pointer duplicated a publication");
    }

    // 双发布模式覆盖非整批外部额度归还、硬上限和跨段 FIFO，重复使用同一批载荷。
    for (size_t mode = 0; mode < 2; ++mode)
    {
        LockFreeQueue<size_t> cachedEntries(false);
        LockFreeQueuePoolConfig cachedConfig;
        cachedConfig.initialCapacity = 4097;
        cachedConfig.maximumCapacity = 4097;
        cachedConfig.maximumProducers = 4;
        bbCheck(cachedEntries.configurePool(cachedConfig) == LockFreeQueueSuccess &&
            cachedEntries.configureEntryOnly(true) == LockFreeQueueSuccess &&
            cachedEntries.configureCancelableReservations(mode != 0) == LockFreeQueueSuccess &&
            cachedEntries.tryInit() == LockFreeQueueSuccess,
            "V5 external credit cache initialization failed");
        std::vector<LockFreeQueueEntry<size_t>> cachedValues(4097);
        for (size_t round = 0; round < 3; ++round)
        {
            LockFreeQueueProducer<size_t> first;
            bbCheck(cachedEntries.tryAcquireProducer(first) == LockFreeQueueSuccess,
                "V5 external credit cache producer unavailable");
            for (size_t i = 0; i < 97; ++i)
            {
                cachedValues[i].m_value = i;
                bbCheck(first.tryPushEntry(cachedValues[i]) == LockFreeQueueSuccess,
                    "V5 partial external credit cache publication failed");
            }
            first.release();
            // 非整批发布后释放租约，所有未用额度必须立即能被下一生产者领取。
            LockFreeQueueProducer<size_t> second;
            bbCheck(cachedEntries.tryAcquireProducer(second) == LockFreeQueueSuccess,
                "V5 replacement external credit cache producer unavailable");
            for (size_t i = 97; i < 4096; ++i)
            {
                cachedValues[i].m_value = i;
                bbCheck(second.tryPushEntry(cachedValues[i]) == LockFreeQueueSuccess,
                    "V5 released external credit cache lost physical capacity");
            }
            bbCheck(second.tryPushEntry(cachedValues[4096]) == LockFreeQueueCapacityLimit,
                "V5 external credit cache exceeded the hard limit");
            second.release();
            for (size_t i = 0; i < 4096; ++i)
            {
                size_t value = 4096;
                bbCheck(cachedEntries.pop(&value) && value == i,
                    "V5 external credit cache changed FIFO or returned a duplicate");
            }
            bbCheck(cachedEntries.empty() && !cachedEntries.pop(nullptr),
                "V5 external credit cache retained phantom items");
        }
    }

    // 空指针、奇地址和小池扩容后的单项归还、丢弃都保持原容量契约。
    LockFreeQueue<char*> mixed(false);
    pointerConfig.initialCapacity = 9;
    pointerConfig.maximumCapacity = 129;
    bbCheck(mixed.configurePool(pointerConfig) == LockFreeQueueSuccess &&
        mixed.configureEntryOnly(true) == LockFreeQueueSuccess && mixed.tryInit() == LockFreeQueueSuccess,
        "V5 mixed pointer initialization failed");
    LockFreeQueueEntry<char*> mixedEntries[128];
    char bytes[128] = {};
    for (size_t round = 0; round < 3; ++round)
    {
        for (size_t i = 0; i < 128; ++i)
        {
            mixedEntries[i].m_value = i % 9 == 0 ? nullptr : &bytes[i];
            bbCheck(mixed.tryPushEntry(mixedEntries[i]) == LockFreeQueueSuccess,
                "V5 pointer fallback growth or reused capacity failed");
        }
        LockFreeQueueEntry<char*> excess;
        bbCheck(mixed.tryPushEntry(excess) == LockFreeQueueCapacityLimit,
            "V5 pointer encoding bypassed the configured quota");
        if (round == 1)
        {
            mixed.clear();
        }
        else
        {
            for (size_t i = 0; i < 128; ++i)
            {
                char* output = nullptr;
                bbCheck(mixed.pop(&output) && output == mixedEntries[i].m_value,
                    "V5 pointer fallback changed address or FIFO");
            }
        }
        bbCheck(mixed.empty(), "V5 pointer fallback failed to return all credits");
    }
}

void taskV5ConcurrentRemovalTest()
{
    CTaskThreadManager& manager = CTaskThreadManager::instance();
    for (size_t round = 0; round < 16; ++round)
    {
        TestWorkers workers;
        for (size_t i = 0; i < 16; ++i)
        {
            workers.add();
        }
        std::atomic<size_t> ready(0);
        std::atomic<bool> start(false);
        std::vector<CTaskThreadResult> results(16);
        std::vector<std::thread> closers;
        closers.reserve(16);
        try
        {
            for (size_t i = 0; i < 16; ++i)
            {
                // 每个关闭者只拥有一个 ID；不修改登记表，真实调用公开关闭接口。
                closers.push_back(std::thread([&, i]()
                {
                    ready.fetch_add(1, std::memory_order_release);
                    while (!start.load(std::memory_order_acquire))
                    {
                        std::this_thread::yield();
                    }
                    results[i] = manager.waitForEnd(workers.ids[i]);
                }));
            }
            bbWait([&ready]()
            {
                return ready.load(std::memory_order_acquire) == 16;
            }, "concurrent removal start barrier");
        }
        catch (...)
        {
            start.store(true, std::memory_order_release);
            for (size_t i = 0; i < closers.size(); ++i)
            {
                closers[i].join();
            }
            throw;
        }
        start.store(true, std::memory_order_release);
        for (size_t i = 0; i < closers.size(); ++i)
        {
            closers[i].join();
        }
        for (size_t i = 0; i < results.size(); ++i)
        {
            bbCheck(results[i].m_status == CTaskThreadOk && results[i].m_committed,
                "unique concurrent close did not finish successfully");
            bbCheck(manager.getThreadInterface(workers.ids[i]) == nullptr,
                "successful close retained a manager entry after a structural CAS conflict");
        }
        bbCheck(manager.count() == 0, "concurrent close retained manager entries");
        workers.drain();
    }
}

CTaskV5RedoFixture::CTaskV5RedoFixture() :
m_holdFactory(false),
m_factoryEntered(false),
m_factoryRelease(false)
{
    for (size_t i = 0; i < 3; ++i)
    {
        m_entered[i].store(false);
        m_release[i].store(false);
        m_factories[i].store(0);
    }
    m_thread = m_workers.add();
}

CTaskV5RedoFixture::~CTaskV5RedoFixture()
{
    for (size_t i = 0; i < 3; ++i)
    {
        m_release[i].store(true);
    }
    m_factoryRelease.store(true);
    try
    {
        m_workers.drain();
    }
    catch (...)
    {
        ++TestTask::errors;
    }
}

std::shared_ptr<TestFactoryTask> CTaskV5RedoFixture::create(size_t slot, int32_t marker, bool blocked, bool redo)
{
    // 无返回值；门控使原任务保持在真实 doTask 内，供外部投递触发抢占。
    std::shared_ptr<TestFactoryTask> task(new TestFactoryTask(marker, [this, slot, marker, blocked](TestTask&)
    {
        m_ledger.add(marker);
        m_entered[slot].store(true);
        if (blocked)
        {
            bbWait([this, slot]()
            {
                return m_release[slot].load();
            }, "V5 original task release");
        }
    }, redo));
    if (redo)
    {
        // 无参数；仅实际重做时调用，返回独立新任务，不预建备份。
        bbCheck(task->clone([this, slot, marker]() -> CTask*
        {
            m_factories[slot].fetch_add(1);
            if (slot == 0 && m_holdFactory.load())
            {
                m_factoryEntered.store(true);
                bbWait([this]()
                {
                    return m_factoryRelease.load();
                }, "V5 redo factory release");
            }
            return new TestFactoryTask(marker, [this, marker](TestTask&)
            {
                m_ledger.add(marker + 1);
            }, false);
        }), "V5 factory registration failed");
    }
    return task;
}

void CTaskV5RedoFixture::post(const std::shared_ptr<CTask>& task, int32_t level)
{
    const std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    for (;;)
    {
        const CTaskThreadResult result = m_thread->postTask(task, level);
        if (result.m_committed)
        {
            bbCheck(result.m_status == CTaskThreadOk, "V5 committed post reported callback failure");
            return;
        }
        bbCheck(result.m_status == CTaskThreadRetry && std::chrono::steady_clock::now() < deadline,
            "V5 uncommitted post failed or timed out");
        std::this_thread::yield();
    }
}

void CTaskV5RedoFixture::waitEntered(size_t slot)
{
    bbWait([this, slot]()
    {
        return m_entered[slot].load();
    }, "V5 original task entered");
}

void CTaskV5RedoFixture::drain()
{
    m_workers.drain();
}

void taskV5PendingRedoPriorityTest()
{
    CTaskV5RedoFixture fixture;
    fixture.post(fixture.create(0, 10, true, true), 1);
    fixture.waitEntered(0);
    fixture.post(fixture.create(1, 20, true, false), 2);
    fixture.post(fixture.create(2, 12, false, false), 1);
    fixture.post(fixture.create(2, 13, false, false), 1);
    fixture.m_release[0].store(true);
    fixture.waitEntered(1);
    bbCheck(fixture.m_factories[0] == 1, "low redo not prepared before middle execution");
    fixture.post(fixture.create(2, 30, false, false), 3);
    fixture.m_release[1].store(true);
    fixture.drain();
    const int expected[] = {10, 20, 30, 11, 12, 13};
    bbCheck(fixture.m_ledger.snapshot() == std::vector<int>(expected, expected + 6),
        "higher task did not precede pending redo or same-level FIFO changed");
    bbCheck(fixture.m_factories[0] == 1 && fixture.m_factories[1] == 0 && fixture.m_factories[2] == 0,
        "unexpected factory invocation while redo was merely queued");
}

void taskV5NestedPreemptionTest()
{
    const int32_t levels[3][3] = {{1, 2, 3}, {1, 16, 17}, {16, 17, INT_MAX}};
    // 同时覆盖缓存目录内、跨越目录上界和通用 Map 的嵌套抢占及两级重做。
    for (size_t scenario = 0; scenario < 3; ++scenario)
    {
        CTaskV5RedoFixture fixture;
        const std::shared_ptr<TestFactoryTask> low = fixture.create(0, 10, true, true);
        const std::shared_ptr<TestFactoryTask> middle = fixture.create(1, 20, true, true);
        fixture.post(low, levels[scenario][0]);
        fixture.waitEntered(0);
        fixture.post(middle, levels[scenario][1]);
        fixture.post(fixture.create(2, 22, false, false), levels[scenario][1]);
        fixture.post(fixture.create(2, 12, false, false), levels[scenario][0]);
        fixture.m_release[0].store(true);
        fixture.waitEntered(1);
        fixture.post(fixture.create(2, 30, false, false), levels[scenario][2]);
        bbCheck(low->stops.load() > 0 && middle->stops.load() > 0, "nested preemption did not notify both running tasks");
        bbCheck(fixture.m_factories[0] == 1 && fixture.m_factories[1] == 0,
            "middle factory invoked before its doTask returned");
        fixture.m_release[1].store(true);
        fixture.drain();
        const int expected[] = {10, 20, 30, 21, 22, 11, 12};
        bbCheck(fixture.m_ledger.snapshot() == std::vector<int>(expected, expected + 7),
            "nested redo priority, redo-front or same-level FIFO violated");
        bbCheck(fixture.m_factories[0] == 1 && fixture.m_factories[1] == 1,
            "nested redo lost or duplicated a factory result");
    }
}

void taskV5QueuedHigherPriorityTest()
{
    CTaskV5RedoFixture fixture;
    fixture.post(fixture.create(0, 10, true, true), 1);
    fixture.waitEntered(0);
    fixture.post(fixture.create(1, 20, false, true), 2);
    fixture.post(fixture.create(2, 30, false, false), 3);
    fixture.post(fixture.create(2, 31, false, false), 3);
    fixture.post(fixture.create(2, 22, false, false), 2);
    fixture.post(fixture.create(2, 12, false, false), 1);
    fixture.m_release[0].store(true);
    fixture.drain();
    const int expected[] = {10, 30, 31, 20, 22, 11, 12};
    bbCheck(fixture.m_ledger.snapshot() == std::vector<int>(expected, expected + 7),
        "already queued higher work was bypassed by lower redo");
    bbCheck(fixture.m_factories[0] == 1 && fixture.m_factories[1] == 0,
        "queued middle task was incorrectly treated as interrupted execution");
}

void taskV5FactoryPublicationRaceTest()
{
    for (int32_t mode = 0; mode < 2; ++mode)
    {
        CTaskV5RedoFixture fixture;
        fixture.m_holdFactory.store(true);
        fixture.post(fixture.create(0, 10, true, true), 1);
        fixture.waitEntered(0);
        fixture.post(fixture.create(1, 20, false, false), 2);
        fixture.m_release[0].store(true);
        bbWait([&fixture]()
        {
            return fixture.m_factoryEntered.load();
        }, "V5 factory entered before later post");
        fixture.post(fixture.create(2, 30, false, false), 3);
        if (mode == 1)
        {
            const CTaskThreadResult result = fixture.m_thread->requestShutdown(false);
            bbCheck(result.m_committed, "discard did not commit during factory");
        }
        fixture.m_factoryRelease.store(true);
        fixture.drain();
        const int expected[] = {10, 30, 20, 11};
        const size_t length = mode == 0 ? 4 : 1;
        bbCheck(fixture.m_ledger.snapshot() == std::vector<int>(expected, expected + length),
            "factory race violated priority or executed discarded work");
        bbCheck(fixture.m_factories[0] == 1, "factory race duplicated creation");
    }
}

void taskV5ReclaimRoutingTest()
{
#ifndef CTTM_TEST_DLL
    const size_t capacities[] = {1, 63, 64, 65, 65535};
    for (size_t c = 0; c < sizeof(capacities) / sizeof(capacities[0]); ++c)
    {
        CTaskStorageBlock block;
        bbCheck(block.initialize(capacities[c]), "V5 state mapping block initialization failed");
        for (size_t i = 0; i < capacities[c]; ++i)
        {
            CTaskStoragePage& page = block.m_pages[i / 64];
            CTaskStorageSlot& slot = page.m_slots[i % 64];
            const uintptr_t address = reinterpret_cast<uintptr_t>(slot.m_state);
            const uintptr_t begin = reinterpret_cast<uintptr_t>(&page.m_states[0]);
            const uintptr_t end = reinterpret_cast<uintptr_t>(&page.m_states[64]);
            bbCheck(address >= begin && address < end && (address - begin) % sizeof(CTaskStorageState) == 0,
                "V5 state mapping points outside its page");
            bbCheck(slot.m_state->m_bits.load() == 0, "V5 state mapping aliases another live record");
            slot.m_state->m_bits.store(static_cast<uint32_t>(i + 1));
        }
        for (size_t i = 0; i < capacities[c]; ++i)
        {
            bbCheck(block.m_pages[i / 64].m_slots[i % 64].m_state->m_bits.load() == i + 1,
                "V5 state mapping overwrote an independent record");
        }
    }
    {
        CTaskRunningIndex running;
        CTaskStorageSlot records[32];
        CTaskStorageState states[32];
        const int32_t levels[] = {1, 4, 2, 3, 5, 8, 6, 7, 9, 12, 10, 11, 13, 16, 14, 15};
        for (size_t i = 0; i < 32; ++i)
        {
            // 模拟唯一消费者：上一项先完成，再发布新执行项；旧登记仍留在目录中。
            if (i != 0)
            {
                states[i - 1].m_bits.store(3);
            }
            records[i].m_state = &states[i];
            records[i].m_level = levels[i < 16 ? i : 31 - i];
            running.publish(records[i], records[i].m_level);
            for (int32_t incoming = 1; incoming <= 18; ++incoming)
            {
                for (int32_t minimum = 1; minimum <= 17; ++minimum)
                {
                    CTaskStorageSlot* expected = records[i].m_level < incoming &&
                        records[i].m_level >= minimum ? &records[i] : nullptr;
                    bbCheck(running.below(incoming, minimum) == expected,
                        "V5 running directory crossed priority bounds or selected completed work");
                }
            }
            states[i].m_bits.store(3);
            bbCheck(running.below(INT_MAX, 1) == nullptr,
                "V5 running directory retained finished work");
            states[i].m_bits.store(0);
        }
        running.clear();
        bbCheck(running.below(INT_MAX, 1) == nullptr,
            "V5 running directory retained reclaimable pointers");
        records[0].m_level = 17;
        running.publish(records[0], records[0].m_level);
        bbCheck(running.below(INT_MAX, 1) == nullptr,
            "V5 uncommon level entered the cached directory");
    }
    {
        CTaskThreadConfig taskConfig;
        CTaskThreadState state(taskConfig);
        CTaskReadDomain& readers = state.*CTaskTestDomain::s_member;
        bbCheck(readers.initialize(4), "V5 completed-current reader initialization failed");
        CTaskStorageSlot record;
        CTaskStorageState recordState;
        record.m_state = &recordState;
        std::shared_ptr<TestTask> task(new TestTask(17, [](TestTask&)
        {

        }));
        record.m_task = task;
        (state.*CTaskTestCurrent::s_member).store(&record);
        record.m_state->m_bits.store(3);
        bbCheck(state.stop(0, 0, true, true).m_status == CTaskThreadOk &&
            state.stop(0, 0, true, false).m_status == CTaskThreadOk && task->stops == 0,
            "V5 retained completed record received a late stop");
        record.m_state->m_bits.store(0);
        bbCheck(state.stop(0, 0, true, true).m_status == CTaskThreadOk && task->stops == 1,
            "V5 active current record lost a stop");
        (state.*CTaskTestCurrent::s_member).store(nullptr);
    }
    CTaskReadDomain domain;
    bbCheck(domain.initialize(4), "V5 reclaim domain initialization failed");
    CTaskStorage storage(domain);
    LockFreeHashMapPoolConfig config;
    config.initialCapacity = 128;
    config.maximumCapacity = 128;
    config.directoryCapacity = 4;
    config.growFreeThreshold = 0;
    bbCheck(storage.initialize(config).m_status == CTaskThreadOk, "V5 reclaim pool initialization failed");
    const int initialLive = TestTask::live.load();
    CTaskReadPosition* held = domain.enter();
    bbCheck(held != nullptr, "V5 reclaim reader registration failed");
    std::vector<CTaskStorageSlot*> slots;
    for (size_t i = 0; i < 128; ++i)
    {
        CTaskThreadResult result;
        CTaskStorageSlot* slot = storage.acquire(storage.cache(i / 64), result);
        bbCheck(slot != nullptr && result.m_status == CTaskThreadOk, "V5 reclaim record allocation failed");
        slot->m_task = std::make_shared<TestTask>(1, [](TestTask&)
        {

        });
        slot->m_state->m_bits.store(3);
        slots.push_back(slot);
        storage.retire(*slot);
    }
    bbCheck(storage.collect(128, true) == 0 && TestTask::live == initialLive + 128,
        "V5 delegated reclaim crossed an old reader");
    domain.leave(*held);
    bbCheck(storage.collect(128, true) == 128 && TestTask::live == initialLive + 128,
        "V5 owner routing lost a batch or reclaimed before help");
    bbCheck(storage.collect(1) == 1 && TestTask::live == initialLive + 127,
        "V5 explicit reclaim exceeded its budget");
    std::atomic<size_t> released(0);
    std::vector<std::thread> helpers;
    helpers.reserve(8);
    try
    {
        for (size_t i = 0; i < 8; ++i)
        {
            helpers.push_back(std::thread([&storage, &released]()
            {
                for (size_t step = 0; step < 64; ++step)
                {
                    released.fetch_add(storage.helpReclaim(storage.cache(step & 1), 1));
                }
            }));
        }
    }
    catch (...)
    {
        for (size_t i = 0; i < helpers.size(); ++i)
        {
            helpers[i].join();
        }
        throw;
    }
    for (size_t i = 0; i < helpers.size(); ++i)
    {
        helpers[i].join();
    }
    released.fetch_add(storage.helpReclaim(storage.cache(0)));
    released.fetch_add(storage.helpReclaim(storage.cache(1)));
    bbCheck(released == 127 && TestTask::live == initialLive && storage.collect(128) == 0,
        "V5 concurrent helpers duplicated or leaked a task reference");
    slots.clear();
    for (size_t i = 0; i < 128; ++i)
    {
        CTaskThreadResult result;
        CTaskStorageSlot* slot = storage.acquire(storage.cache(i / 64), result);
        bbCheck(slot != nullptr && result.m_status == CTaskThreadOk, "V5 helped records did not restore bounded capacity");
        bbCheck(std::find(slots.begin(), slots.end(), slot) == slots.end(), "V5 helper returned a record twice");
        slots.push_back(slot);
    }
    for (size_t i = 0; i < slots.size(); ++i)
    {
        storage.abandon(*slots[i]);
    }
    {
        CTaskReadDomain boundaryDomain;
        bbCheck(boundaryDomain.initialize(4), "V5 boundary domain initialization failed");
        CTaskStorage boundaryStorage(boundaryDomain);
        LockFreeHashMapPoolConfig boundaryConfig;
        boundaryConfig.initialCapacity = 8192;
        boundaryConfig.maximumCapacity = 8192;
        boundaryConfig.growFreeThreshold = 0;
        bbCheck(boundaryStorage.initialize(boundaryConfig).m_status == CTaskThreadOk,
            "V5 boundary pool initialization failed");
        CTaskReadPosition* partialReader = nullptr;
        for (size_t i = 0; i < 80; ++i)
        {
            if (i == 63)
            {
                boundaryDomain.advance();
                partialReader = boundaryDomain.tryEnter(3);
                bbCheck(partialReader != nullptr, "V5 partial-prefix reader initialization failed");
            }
            CTaskThreadResult result;
            CTaskStorageSlot* slot = boundaryStorage.acquire(boundaryStorage.cache(0), result);
            bbCheck(slot != nullptr && result.m_status == CTaskThreadOk,
                "V5 partial-prefix record allocation failed");
            slot->m_task = std::make_shared<TestTask>(626, [](TestTask&)
            {

            });
            slot->m_state->m_bits.store(3);
            boundaryStorage.retire(*slot);
        }
        bbCheck(boundaryStorage.collect(80) == 63 && TestTask::live == initialLive + 17,
            "V5 partial-prefix reclaim crossed or stopped before the exact protection boundary");
        boundaryDomain.leave(*partialReader);
        bbCheck(boundaryStorage.collect(80) == 17 && TestTask::live == initialLive,
            "V5 partial-prefix tail was lost after its reader left");
        const size_t counts[] = {1023, 1024, 1031, 4095, 4096, 4103};
        for (size_t round = 0; round < sizeof(counts) / sizeof(counts[0]); ++round)
        {
            CTaskReadPosition* protectedReader = boundaryDomain.tryEnter(3);
            bbCheck(protectedReader != nullptr, "V5 boundary reader initialization failed");
            std::vector<CTaskStorageSlot*> records;
            for (size_t i = 0; i < counts[round]; ++i)
            {
                CTaskThreadResult result;
                CTaskStorageSlot* slot = boundaryStorage.acquire(boundaryStorage.cache(i % 3), result);
                bbCheck(slot != nullptr && result.m_status == CTaskThreadOk,
                    "V5 boundary record allocation failed");
                slot->m_task = std::make_shared<TestTask>(626, [](TestTask&)
                {

                });
                slot->m_state->m_bits.store(3);
                records.push_back(slot);
            }
            for (size_t i = 0; i < records.size(); ++i)
            {
                boundaryStorage.retire(*records[i]);
            }
            bbCheck(boundaryStorage.collect(counts[round], true) == 0 &&
                TestTask::live == initialLive + static_cast<int>(counts[round]),
                "V5 boundary reclamation crossed a protected reader");
            boundaryDomain.leave(*protectedReader);
            bbCheck(boundaryStorage.collect(counts[round], true) == counts[round],
                "V5 boundary retirement lost the window or overflow tail");
            const int beforeHelp = TestTask::live.load();
            size_t helped = 0;
            for (size_t owner = 0; owner < 3; ++owner)
            {
                const size_t count = boundaryStorage.helpReclaim(boundaryStorage.cache(owner), 1024);
                bbCheck(count != 0, "V5 boundary retirement routed no records to a live owner");
                helped += count;
            }
            bbCheck(boundaryStorage.helpReclaim(boundaryStorage.cache(3), 1024) == 0,
                "V5 boundary retirement routed a record to the wrong owner");
            helped += boundaryStorage.collect(counts[round]);
            bbCheck(helped == static_cast<size_t>(beforeHelp - initialLive) && TestTask::live == initialLive &&
                boundaryStorage.collect(counts[round]) == 0,
                "V5 boundary retirement duplicated or retained a task reference");
        }
    }
#endif
}

void taskV5RepeatedRedoTest()
{
    const uint32_t rounds = 64;
    TaskLedger ledger;
    std::atomic<uint32_t> entered(0);
    std::atomic<uint32_t> released(0);
    std::atomic<uint32_t> factories(0);
    // 参数为代次；生成任务的执行体在外部放行前保持在 doTask 内。
    const std::function<CTask*(uint32_t)> make = [&](uint32_t generation) -> CTask*
    {
        return new TestFactoryTask(91, [&, generation](TestTask&)
        {
            ledger.add(100 + generation);
            entered.store(generation + 1);
            bbWait([&, generation]()
            {
                return released.load() > generation;
            }, "V5 repeated redo generation release");
        }, true);
    };
    std::shared_ptr<CTask> original(make(0));
    bbCheck(original->clone([&]() -> CTask*
    {
        const uint32_t generation = factories.fetch_add(1) + 1;
        return make(generation);
    }), "V5 repeated redo factory registration failed");
    TestWorkers workers;
    std::shared_ptr<CTaskThread> worker = workers.add();
    try
    {
        bbCheck(worker->postTask(original, 1).m_committed, "V5 repeated original rejected");
        bbCheck(worker->postTask(std::make_shared<TestTask>(91, [&](TestTask&)
        {
            ledger.add(9999);
        }), 1).m_committed, "V5 repeated follower rejected");
        for (uint32_t round = 0; round < rounds; ++round)
        {
            bbWait([&]()
            {
                return entered.load() == round + 1;
            }, "V5 repeated generation entered");
            bbCheck(factories.load() == round, "V5 factory ran before this generation returned");
            bbCheck(worker->postTask(std::make_shared<TestTask>(92, [&, round](TestTask&)
            {
                ledger.add(2000 + round);
            }), 2).m_committed, "V5 repeated middle rejected");
            bbCheck(worker->postTask(std::make_shared<TestTask>(93, [&, round](TestTask&)
            {
                ledger.add(3000 + round);
            }), 3).m_committed, "V5 repeated higher rejected");
            released.store(round + 1);
        }
        bbWait([&]()
        {
            return entered.load() == rounds + 1;
        }, "V5 final generation entered");
        released.store(UINT32_MAX);
        workers.drain();
    }
    catch (...)
    {
        released.store(UINT32_MAX);
        workers.drain();
        throw;
    }
    std::vector<int> expected;
    for (uint32_t round = 0; round < rounds; ++round)
    {
        expected.push_back(100 + round);
        expected.push_back(3000 + round);
        expected.push_back(2000 + round);
    }
    expected.push_back(100 + rounds);
    expected.push_back(9999);
    bbCheck(ledger.snapshot() == expected, "V5 repeated redo lost its original priority or queue head");
    bbCheck(factories.load() == rounds && worker->statistics().m_errors == 0 &&
        worker->statistics().m_outstanding == 0, "V5 repeated redo leaked work or duplicated factories");
}

/** 门控等待只检查本轮协议，不向生产库嵌入测试钩子
@param [in] predicate 观察原子门控的判断函数
@param [in] reason 超时说明
*/
static void TaskV5SpinUntil(const std::function<bool()>& predicate, const char* reason)
{
    const uint64_t deadline = bbNow() + 5000;
    size_t iterations = 0;
    while (!predicate())
    {
        if ((++iterations & 1023) == 0)
        {
            bbCheck(bbNow() < deadline, reason);
            std::this_thread::yield();
        }
    }
}

/** 同一工作协议下测量稀疏重做；创建普通任务、原任务和工厂均在计时前
@param [in] redo 是否在每次合作打断后真正生成一个重做任务
@return 一百万普通任务和一百轮打断从首个投递到最后执行的毫秒数
*/
static double TaskV5MeasureRedo(bool redo)
{
    const size_t rounds = 100;
    const size_t perRound = 10000;
    std::atomic<size_t> entered(0);
    std::atomic<size_t> completedRounds(0);
    std::atomic<bool> cancel(false);
    size_t executed = 0;
    size_t factories = 0;
    size_t repeated = 0;
    size_t highExecuted = 0;
    std::chrono::steady_clock::time_point finished;
    std::vector<std::shared_ptr<CTask>> ordinary;
    std::vector<std::shared_ptr<CTask>> originals;
    std::vector<std::shared_ptr<CTask>> markers;
    for (size_t i = 0; i < 2000; ++i)
    {
        ordinary.push_back(std::make_shared<TestTask>(610, [&](TestTask&)
        {
            ++executed;
        }));
    }
    const std::shared_ptr<CTask> high = std::make_shared<TestTask>(611, [&](TestTask&)
    {
        ++highExecuted;
    });
    for (size_t i = 0; i < rounds; ++i)
    {
        std::shared_ptr<TestFactoryTask> original(new TestFactoryTask(612, [&, i](TestTask& task)
        {
            entered.store(i + 1, std::memory_order_release);
            TaskV5SpinUntil([&]()
            {
                return task.stopped.load(std::memory_order_acquire) || cancel.load(std::memory_order_acquire);
            }, "V5 redo performance original never received stopTask");
        }, redo));
        bbCheck(original->clone([&]() -> CTask*
        {
            ++factories;
            return new TestTask(613, [&](TestTask&)
            {
                ++repeated;
            });
        }), "V5 redo performance factory configuration failed");
        originals.push_back(original);
        markers.push_back(std::make_shared<TestTask>(614, [&, i](TestTask&)
        {
            bbCheck(executed == (i + 1) * perRound && highExecuted == i + 1,
                "V5 redo performance ordinary/high execution order mismatch");
            bbCheck(factories == (redo ? i + 1 : 0) && repeated == factories,
                "V5 redo performance factory ran early, twice, or after same-level follower");
            if (i + 1 == rounds)
            {
                finished = std::chrono::steady_clock::now();
            }
            completedRounds.store(i + 1, std::memory_order_release);
        }));
    }
    TestWorkers workers;
    CTaskThreadConfig config;
    config.m_taskMap.initialCapacity = perRound * 2;
    config.m_queue.initialCapacity = perRound * 2;
    const uint32_t id = CTaskThreadManager::instance().init(config);
    bbCheck(id != 0, "V5 redo performance initialization failed");
    workers.ids.push_back(id);
    const std::shared_ptr<CTaskThread> worker = CTaskThreadManager::instance().getThreadInterface(id);
    const std::function<void(const std::shared_ptr<CTask>&, int32_t)> post =
        [&](const std::shared_ptr<CTask>& task, int32_t level)
    {
        CTaskThreadResult result;
        do
        {
            result = worker->postTask(task, level);
        } while (!result.m_committed && result.m_status == CTaskThreadRetry);
        bbCheck(result.m_committed && result.m_status == CTaskThreadOk, "V5 redo performance post failed");
    };
    const std::chrono::steady_clock::time_point started = std::chrono::steady_clock::now();
    try
    {
        for (size_t i = 0; i < rounds; ++i)
        {
            for (size_t j = 0; j < perRound; ++j)
            {
                post(ordinary[j % ordinary.size()], 1);
            }
            post(originals[i], 1);
            TaskV5SpinUntil([&]()
            {
                return entered.load(std::memory_order_acquire) == i + 1;
            }, "V5 redo performance original did not start");
            post(high, 2);
            post(markers[i], 1);
            TaskV5SpinUntil([&]()
            {
                return completedRounds.load(std::memory_order_acquire) == i + 1;
            }, "V5 redo performance round did not complete");
        }
        workers.drain();
    }
    catch (...)
    {
        cancel.store(true, std::memory_order_release);
        workers.drain();
        throw;
    }
    bbCheck(executed == rounds * perRound && factories == (redo ? rounds : 0) && repeated == factories &&
        worker->statistics().m_outstanding == 0 && worker->statistics().m_errors == 0,
        "V5 redo performance lost tasks, leaked admission, or failed a callback");
    return std::chrono::duration<double, std::milli>(finished - started).count();
}

void taskV5RedoPerformanceTest()
{
    double baseline[3];
    double redo[3];
    for (size_t round = 0; round < 3; ++round)
    {
        // 交替次序，减少升频和热缓存偏向某一个方案。
        std::cout << "redo_performance starting_pair=" << round + 1 << std::endl;
        if ((round & 1) == 0)
        {
            baseline[round] = TaskV5MeasureRedo(false);
            redo[round] = TaskV5MeasureRedo(true);
        }
        else
        {
            redo[round] = TaskV5MeasureRedo(true);
            baseline[round] = TaskV5MeasureRedo(false);
        }
        std::cout << std::fixed << std::setprecision(6) << "redo_performance round=" << round + 1
            << " ordinary=1000000 interruptions=100 baseline_ms=" << baseline[round]
            << " redo_ms=" << redo[round] << " extra_factories=100 extra_executions=100" << std::endl;
    }
    std::sort(baseline, baseline + 3);
    std::sort(redo, redo + 3);
    std::cout << "redo_performance median_baseline_ms=" << baseline[1] << " median_redo_ms=" << redo[1]
        << " elapsed_change_percent=" << (redo[1] / baseline[1] - 1.0) * 100.0 << '\n';
}

/** 只重试明确未提交的临时失败，避免测试在已提交通知失败时重复投递
@param [in] worker 本轮独立工作线程
@param [in] task 本轮不可变任务
@param [in] level 优先级
*/
static void TaskV5PostChecked(const std::shared_ptr<CTaskThread>& worker,
    const std::shared_ptr<CTask>& task, int32_t level)
{
    CTaskThreadResult result;
    TaskV5SpinUntil([&]()
    {
        result = worker->postTask(task, level);
        return result.m_committed || result.m_status != CTaskThreadRetry;
    }, "V5 race post remained unavailable");
    bbCheck(result.m_committed && result.m_status == CTaskThreadOk, "V5 race post failed");
}

void taskV5FirstPriorityRaceTest()
{
    for (size_t mode = 0; mode < 2; ++mode)
    {
        const int32_t levels[] = {1, 2, 4, 8, 16, 3, 5, 9, 6, 10, 12, 7, 11, 13, 14, 15, 1};
        const size_t total = sizeof(levels) / sizeof(levels[0]);
        std::atomic<size_t> completed(0);
        std::atomic<bool> failed(false);
        std::atomic<int32_t> faultCalls(0);
        std::atomic<bool> faultEntered(false);
        std::atomic<bool> faultRelease(true);
        TestWorkers workers;
        const std::shared_ptr<CTaskThread> worker = workers.add();
        for (size_t i = 0; i < total; ++i)
        {
            const int32_t level = mode == 1 && i == 3 ? 17 : (mode == 1 && i == 5 ? INT_MAX : levels[i]);
            // 从公开接口连续切换级别，覆盖正常完成、异常完成及通用级别路径。
            TaskV5PostChecked(worker, std::make_shared<TestTask>(623, [&, i, level](TestTask&)
            {
                if (worker->getCurTaskLevel() != level)
                {
                    failed.store(true, std::memory_order_release);
                }
                completed.store(i + 1, std::memory_order_release);
            }), level);
            TaskV5SpinUntil([&]() { return completed.load(std::memory_order_acquire) == i + 1; },
                "V5 priority handoff did not execute the next task");
            if (i == 6)
            {
                // TestTask 会捕获自身断言；用既有故障任务将异常实际交给工作线程处理。
                TaskV5PostChecked(worker, std::make_shared<TestFaultTask>(1, faultCalls, faultEntered, faultRelease), level);
                TaskV5SpinUntil([&]() { return faultEntered.load(std::memory_order_acquire); },
                    "V5 priority handoff exception task did not start");
            }
        }
        workers.drain();
        bbCheck(!failed.load() && completed.load() == total && worker->getCurTaskLevel() == 0 &&
            worker->statistics().m_executed == total + 1 && worker->statistics().m_errors == 1 && faultCalls == 1,
            "V5 priority handoff retained an earlier level or lost exception recovery");
    }
    for (size_t round = 0; round < 128; ++round)
    {
        std::atomic<bool> entered(false);
        std::atomic<bool> release(false);
        std::atomic<bool> higherExecuted(false);
        std::atomic<bool> failed(false);
        size_t lowerExecutions = 0;
        TestWorkers workers;
        const std::shared_ptr<CTaskThread> worker = workers.add();
        std::thread publisher;
        try
        {
            TaskV5PostChecked(worker, std::make_shared<TestTask>(620, [&](TestTask&)
            {
                entered.store(true, std::memory_order_release);
                TaskV5SpinUntil([&]() { return release.load(std::memory_order_acquire); },
                    "V5 first priority gate was not released");
            }), 1);
            TaskV5SpinUntil([&]() { return entered.load(std::memory_order_acquire); },
                "V5 first priority gate did not start");
            TaskV5PostChecked(worker, std::make_shared<TestTask>(621, [&](TestTask& task)
            {
                ++lowerExecutions;
                // 两种合法顺序：高级已经先执行，或者低级先执行并收到高级的通知。
                TaskV5SpinUntil([&]()
                {
                    return higherExecuted.load(std::memory_order_acquire) || task.stopped.load();
                }, "V5 first higher publication lost its preemption notification");
            }), 1);
            const std::shared_ptr<CTask> higher = std::make_shared<TestTask>(622, [&](TestTask&)
            {
                higherExecuted.store(true, std::memory_order_release);
            });
            publisher = std::thread([&, higher]()
            {
                try
                {
                    TaskV5SpinUntil([&]() { return release.load(std::memory_order_acquire); },
                        "V5 higher publisher was not released");
                    TaskV5PostChecked(worker, higher, round % 3 == 0 ? 17 : 2);
                }
                catch (...)
                {
                    failed.store(true);
                }
            });
            release.store(true, std::memory_order_release);
            publisher.join();
            workers.drain();
            bbCheck(!failed.load() && higherExecuted.load() && lowerExecutions == 1 &&
                worker->statistics().m_errors == 0 && worker->statistics().m_outstanding == 0,
                "V5 first priority race lost work or leaked admission");
        }
        catch (...)
        {
            release.store(true, std::memory_order_release);
            if (publisher.joinable())
            {
                publisher.join();
            }
            workers.drain();
            throw;
        }
    }
}

void taskV5ReclaimBatchesTest()
{
#ifndef CTTM_TEST_DLL
    CTaskReadDomain domain;
    bbCheck(domain.initialize(2), "V5 reclaim batches domain initialization failed");
    CTaskStorage storage(domain);
    LockFreeHashMapPoolConfig config;
    config.initialCapacity = 512;
    config.maximumCapacity = 512;
    config.growFreeThreshold = 0;
    bbCheck(storage.initialize(config).m_status == CTaskThreadOk, "V5 reclaim batches pool failed");
    const int initialLive = TestTask::live.load();
    CTaskStorageCache& cache = storage.cache(0);
    for (size_t round = 0; round < 3; ++round)
    {
        std::vector<CTaskStorageSlot*> slots;
        for (size_t i = 0; i < 320; ++i)
        {
            CTaskThreadResult result;
            CTaskStorageSlot* slot = storage.acquire(cache, result);
            bbCheck(slot != nullptr && result.m_status == CTaskThreadOk, "V5 reclaim batches allocation failed");
            slot->m_task = std::make_shared<TestTask>(625, [](TestTask&) {});
            slot->m_state->m_bits.store(3);
            slots.push_back(slot);
        }
        for (size_t i = 0; i < slots.size(); ++i)
        {
            storage.retire(*slots[i]);
        }
        bbCheck(storage.collect(320, true) == 320 && TestTask::live == initialLive + 256,
            "V5 full reclaim batches failed to release the overflow directly");
        // 只在测试模型中停在帮助者取得批次之后；其余真实帮助入口仍应前进。
        int32_t count = 64;
        bbCheck(cache.m_reclaims[0].m_count.compare_exchange_strong(count, -1),
            "V5 reclaim pause model failed to claim its batch");
        bbCheck(storage.helpReclaim(cache, 256) == 192 && TestTask::live == initialLive + 64,
            "V5 paused helper blocked independent batches");
        cache.m_reclaims[0].m_count.store(64, std::memory_order_release);
        for (size_t i = 0; i < 64; ++i)
        {
            bbCheck(storage.helpReclaim(cache, 1) == 1 && TestTask::live == initialLive + 63 - i,
                "V5 partial reclaim changed its budget or duplicated release");
        }
        bbCheck(storage.helpReclaim(cache) == 0 && storage.collect(512) == 0 && TestTask::live == initialLive,
            "V5 reclaimed batch retained a task or appeared twice");
    }
    // 不足一批的尾部也必须遵守显式回收预算，并在消费者空闲维护时释放。
    const size_t tails[] = {1, 15, 16, 31, 32, 63, 64};
    for (size_t group = 0; group < sizeof(tails) / sizeof(tails[0]); ++group)
    {
        const size_t count = tails[group];
        for (size_t i = 0; i < count; ++i)
        {
            CTaskThreadResult result;
            CTaskStorageSlot* slot = storage.acquire(cache, result);
            bbCheck(slot != nullptr && result.m_status == CTaskThreadOk,
                "V5 partial reclaim batch allocation failed");
            slot->m_task = std::make_shared<TestTask>(625, [](TestTask&) {});
            slot->m_state->m_bits.store(3);
            storage.retire(*slot);
        }
        bbCheck(storage.collect(count, true) == count && TestTask::live == initialLive + count,
            "V5 partial delegated batch lost a task reference");
        bbCheck(storage.collect(1) == 1 && TestTask::live == initialLive + count - 1,
            "V5 partial tail reclamation exceeded its explicit budget");
        bbCheck(storage.collect(512) == count - 1 && TestTask::live == initialLive,
            "V5 idle reclamation retained the partial tail");
    }

#endif
}

void taskV5CurrentPropertyStopTest()
{
    std::atomic<bool> entered(false);
    std::atomic<bool> release(false);
    size_t completed = 0;
    TestWorkers workers;
    const std::shared_ptr<CTaskThread> worker = workers.add();
    const std::shared_ptr<TestTask> current = std::make_shared<TestTask>(630, [&](TestTask&)
    {
        entered.store(true, std::memory_order_release);
        TaskV5SpinUntil([&]() { return release.load(std::memory_order_acquire); },
            "V5 current property stop gate was not released");
        ++completed;
    });
    const std::shared_ptr<TestTask> matching = std::make_shared<TestTask>(630, [&](TestTask&) { ++completed; });
    const std::shared_ptr<TestTask> differentLevel = std::make_shared<TestTask>(630, [&](TestTask&) { ++completed; });
    const std::shared_ptr<TestTask> differentProperty = std::make_shared<TestTask>(631, [&](TestTask&) { ++completed; });
    try
    {
        TaskV5PostChecked(worker, current, 10);
        TaskV5SpinUntil([&]() { return entered.load(std::memory_order_acquire); },
            "V5 current property stop task did not start");
        TaskV5PostChecked(worker, matching, 1);
        TaskV5PostChecked(worker, matching, 1);
        TaskV5PostChecked(worker, differentLevel, 2);
        TaskV5PostChecked(worker, differentProperty, 1);
        bbCheck(worker->stopTask(630, 1).m_status == CTaskThreadOk && current->stops == 1 &&
            matching->stops == 1 && differentLevel->stops == 0 && differentProperty->stops == 0,
            "V5 current identity or queued property/priority selection changed");
        bbCheck(worker->getWaitTaskCount() == 4, "V5 property stop removed queued work");
        release.store(true, std::memory_order_release);
        workers.drain();
        bbCheck(completed == 5 && worker->statistics().m_errors == 0,
            "V5 current property stop lost work or recorded a callback error");
    }
    catch (...)
    {
        release.store(true, std::memory_order_release);
        workers.drain();
        throw;
    }
}

void taskV5ShutdownHandoffTest()
{
    for (size_t round = 0; round < 256; ++round)
    {
        std::atomic<bool> entered(false);
        std::atomic<bool> release(false);
        std::atomic<bool> failed(false);
        size_t executions = 0;
        TestWorkers workers;
        const std::shared_ptr<CTaskThread> worker = workers.add();
        std::thread stopper;
        try
        {
            TaskV5PostChecked(worker, std::make_shared<TestTask>(623, [&](TestTask&)
            {
                entered.store(true, std::memory_order_release);
                TaskV5SpinUntil([&]() { return release.load(std::memory_order_acquire); },
                    "V5 shutdown gate was not released");
            }), 1);
            TaskV5SpinUntil([&]() { return entered.load(std::memory_order_acquire); },
                "V5 shutdown gate did not start");
            TaskV5PostChecked(worker, std::make_shared<TestTask>(624, [&](TestTask& task)
            {
                ++executions;
                TaskV5SpinUntil([&]() { return task.stopped.load(std::memory_order_acquire); },
                    "V5 discard shutdown missed task handoff");
            }), 1);
            stopper = std::thread([&]()
            {
                try
                {
                    TaskV5SpinUntil([&]() { return release.load(std::memory_order_acquire); },
                        "V5 shutdown caller was not released");
                    if (worker->requestShutdown(false).m_status != CTaskThreadOk)
                    {
                        failed.store(true);
                    }
                }
                catch (...)
                {
                    failed.store(true);
                }
            });
            release.store(true, std::memory_order_release);
            stopper.join();
            // 等待本次关闭自行结束，再 join；不允许额外一次停止掩盖丢通知。
            TaskV5SpinUntil([&]() { return worker->statistics().m_exited; },
                "V5 discard shutdown did not finish without a second notification");
            workers.drain();
            bbCheck(!failed.load() && executions <= 1 && worker->statistics().m_errors == 0 &&
                worker->statistics().m_outstanding == 0, "V5 shutdown handoff failed or leaked work");
        }
        catch (...)
        {
            release.store(true, std::memory_order_release);
            if (stopper.joinable())
            {
                stopper.join();
            }
            workers.drain();
            throw;
        }
    }
}