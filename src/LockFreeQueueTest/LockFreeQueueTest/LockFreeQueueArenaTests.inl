#ifndef LOCK_FREE_QUEUE_ARENA_TESTS_INL
#define LOCK_FREE_QUEUE_ARENA_TESTS_INL

/** 记录独立预期失败并继续清理
@param [in] condition 独立预期的实际结果
@param [in] name 非空用例说明字符串
@return 全部独立预期是否成立
*/
static bool LockFreeQueueArenaCheck(bool condition, const char* name)
{
    if (!condition)
    {
        std::cout << "FAIL " << name << std::endl;
    }
    return condition;
}

/** 三次填满有限池、精确拒绝、预算一真实缩容及再增长
@param [in] threshold 0 或 8 的增长阈值
@return 全部独立预期是否成立
*/
static bool LockFreeQueueArenaCapacities(size_t threshold)
{
    LockFreeQueueArena<uint64_t> pool;
    LockFreeQueuePoolConfig config;
    config.initialCapacity = 64;
    config.maximumCapacity = 4096;
    config.maximumProducers = 16;
    config.growThreshold = threshold;
    bool ok = LockFreeQueueArenaCheck(pool.configure(config) == LockFreeQueueSuccess, "configure");
    ok = LockFreeQueueArenaCheck(pool.initialize() == LockFreeQueueSuccess, "initialize") && ok;
    typedef LockFreeQueueArena<uint64_t>::Node Node;
    auto record = pool.enter();
    if (!LockFreeQueueArenaCheck(record != nullptr, "record"))
    {
        return false;
    }
    std::vector<Node*> nodes;
    for (size_t cycle = 0; cycle < 3; ++cycle)
    {
        const auto before = pool.statistics();
        for (size_t i = 0; i < 4095; ++i)
        {
            LockFreeQueueResult result = LockFreeQueueSuccess;
            Node* node = pool.acquireNode(*record, result);
            if (!LockFreeQueueArenaCheck(node && result == LockFreeQueueSuccess, "acquire within finite bound"))
            {
                return false;
            }
            node->construct(i);
            nodes.push_back(node);
            if (cycle == 0 && i == 62)
            {
                ok = LockFreeQueueArenaCheck(
                         (pool.statistics().successfulGrowCount == 0) == (threshold == 0), "threshold boundary") &&
                     ok;
            }
        }
        LockFreeQueueResult result = LockFreeQueueSuccess;
        Node* rejected = pool.acquireNode(*record, result);
        ok = LockFreeQueueArenaCheck(
                 rejected == nullptr && result == LockFreeQueueCapacityLimit, "exact maximum refusal") &&
             ok;
        const auto filled = pool.statistics();
        ok = LockFreeQueueArenaCheck(
                 filled.currentCapacity == 4096 && filled.peakReservedCapacity == 4096, "physical bound") &&
             ok;
        if (cycle != 0)
        {
            ok = LockFreeQueueArenaCheck(filled.allocationCalls == before.allocationCalls &&
                                             filled.successfulGrowCount == before.successfulGrowCount,
                     "grown pool reuse no allocation") &&
                 ok;
        }
        for (size_t i = 0; i < nodes.size(); ++i)
        {
            ok = LockFreeQueueArenaCheck(*nodes[i]->data() == i, "payload intact") && ok;
            pool.releaseNode(nodes[i]);
        }
        nodes.clear();
        ok = LockFreeQueueArenaCheck(pool.statistics().reusableFreeCapacity == 4095, "all nodes returned") && ok;
    }
    pool.releaseCache(*record);
    pool.leave(*record, true);
    pool.setShrinkPolicy(
        [](const LockFreeQueueShrinkContext& context)
        {
            LockFreeQueueShrinkDecision decision = {context.currentCapacity, 1};
            return decision;
        });
    for (size_t i = 0; i < 512; ++i)
    {
        pool.maintain(1);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(4));
    size_t released = 0;
    for (size_t i = 0; i < 512; ++i)
    {
        released += pool.maintain(1).releasedCapacity;
    }
    const auto shrunk = pool.statistics();
    ok = LockFreeQueueArenaCheck(
             shrunk.currentCapacity == 64 && shrunk.reusableFreeCapacity == 63, "budget-one actual shrink") &&
         ok;
    ok = LockFreeQueueArenaCheck(
             shrunk.retiringBlockCapacity == 0 && shrunk.reservedCapacity == 0, "no pending reservations") &&
         ok;
    record = pool.enter();
    for (size_t i = 0; i < 100; ++i)
    {
        LockFreeQueueResult result = LockFreeQueueSuccess;
        Node* node = pool.acquireNode(*record, result);
        if (!LockFreeQueueArenaCheck(node != nullptr, "regrow"))
        {
            return false;
        }
        nodes.push_back(node);
    }
    for (size_t i = 0; i < nodes.size(); ++i)
    {
        pool.releaseNode(nodes[i]);
    }
    pool.releaseCache(*record);
    pool.leave(*record, true);
    ok = LockFreeQueueArenaCheck(
             pool.statistics().successfulGrowCount > shrunk.successfulGrowCount, "regrow cumulative count") &&
         ok;
    std::cout << "ARENA capacity threshold=" << threshold << " released_after_wait=" << released
              << " allocations=" << pool.statistics().allocationCalls << " valid=" << ok << std::endl;
    return ok;
}

/** 检查空策略、固定增长、异常和非法容量边界
@return 全部独立预期是否成立
*/
static bool LockFreeQueueArenaPolicies()
{
    LockFreeQueueArena<int> pool;
    LockFreeQueuePoolConfig config;
    config.initialCapacity = 2;
    config.maximumCapacity = -1;
    config.maximumProducers = 1;
    bool ok = LockFreeQueueArenaCheck(
        pool.configure(config) == LockFreeQueueSuccess && pool.initialize() == LockFreeQueueSuccess, "policy init");
    auto record = pool.enter();
    LockFreeQueueResult result = LockFreeQueueSuccess;
    auto first = pool.acquireNode(*record, result);
    ok = LockFreeQueueArenaCheck(first != nullptr, "single usable node") && ok;
    pool.setGrowPolicy([](const LockFreeQueueGrowContext&) -> size_t { return 0; });
    ok = LockFreeQueueArenaCheck(
             pool.acquireNode(*record, result) == nullptr && result == LockFreeQueueGrowthDeclined, "decline") &&
         ok;
    pool.setGrowPolicy([](const LockFreeQueueGrowContext&) -> size_t { throw 7; });
    ok = LockFreeQueueArenaCheck(pool.acquireNode(*record, result) == nullptr && result == LockFreeQueuePolicyFailure,
             "policy exception") &&
         ok;
    pool.setGrowPolicy([](const LockFreeQueueGrowContext&) -> size_t { return 7; });
    auto second = pool.acquireNode(*record, result);
    ok = LockFreeQueueArenaCheck(second != nullptr && pool.statistics().currentCapacity == 9, "fixed increment") && ok;
    if (first)
    {
        pool.releaseNode(first, record);
    }
    if (second)
    {
        pool.releaseNode(second, record);
    }
    pool.releaseCache(*record);
    pool.leave(*record, true);
    pool.destroy();
    pool.destroy();
    config.maximumCapacity = -2;
    ok = LockFreeQueueArenaCheck(pool.configure(config) == LockFreeQueueInvalidConfiguration, "invalid negative") && ok;
    config.maximumCapacity = 1;
    ok =
        LockFreeQueueArenaCheck(pool.configure(config) == LockFreeQueueInvalidConfiguration, "maximum below initial") &&
        ok;
    config.maximumCapacity = -1;
    config.initialCapacity = 0;
    ok = LockFreeQueueArenaCheck(pool.configure(config) == LockFreeQueueInvalidConfiguration, "zero initial") && ok;
    return ok;
}

/** 十六线程独立核对一百六十万次节点领取、载荷和归还
@return 全部独立预期是否成立
*/
static bool LockFreeQueueArenaConcurrentClaims()
{
    LockFreeQueueArena<uint64_t> pool;
    LockFreeQueuePoolConfig config;
    config.initialCapacity = 8192;
    config.maximumCapacity = 8192;
    config.maximumProducers = 16;
    if (!LockFreeQueueArenaCheck(
            pool.configure(config) == LockFreeQueueSuccess && pool.initialize() == LockFreeQueueSuccess,
            "concurrent init"))
    {
        return false;
    }
    std::atomic<size_t> errors(0), ready(0);
    std::atomic<bool> go(false), stop(false);
    LockFreeQueueThreadGroup group(stop, &go);
    std::vector<std::thread>& workers = group.m_threads;
    workers.reserve(16);
    for (size_t p = 0; p < 16; ++p)
    {
        workers.push_back(std::thread(
            [&, p]
            {
                auto record = pool.enter();
                ready.fetch_add(1);
                while (!go.load())
                {
                    std::this_thread::yield();
                }
                if (!record)
                {
                    errors.fetch_add(1);
                    return;
                }
                for (uint64_t i = 0; i < 100000 && !stop.load(); ++i)
                {
                    LockFreeQueueResult result = LockFreeQueueSuccess;
                    auto node = pool.acquireNode(*record, result);
                    if (!node)
                    {
                        errors.fetch_add(1);
                        continue;
                    }
                    const uint64_t value = (uint64_t(p) << 48) | i;
                    node->construct(value);
                    if (*node->data() != value)
                    {
                        errors.fetch_add(1);
                    }
                    pool.releaseNode(node, record);
                }
                pool.releaseCache(*record);
                pool.leave(*record, true);
            }));
    }
    while (ready.load() != 16)
    {
        std::this_thread::yield();
    }
    go.store(true);
    group.join();
    const auto stats = pool.statistics();
    const bool ok = errors.load() == 0 && stats.currentCapacity == 8192 && stats.reusableFreeCapacity == 8191 &&
                    stats.allocationCalls == 2 && stats.activeProducers == 0;
    std::cout << "ARENA concurrency count=1600000 errors=" << errors.load() << " valid=" << ok << std::endl;
    return ok;
}

/** 混合未构造存储、私有缓存及两千项共享载荷后检查析构计数
@return 全部独立预期是否成立
*/
static bool LockFreeQueueArenaLazyLifetimes()
{
    typedef LockFreeQueueArena<std::shared_ptr<int>> Arena;
    Arena pool;
    LockFreeQueuePoolConfig config;
    config.initialCapacity = 65;
    config.maximumCapacity = 4096;
    config.maximumProducers = 3;
    bool ok = pool.configure(config) == LockFreeQueueSuccess && pool.initialize() == LockFreeQueueSuccess;
    Arena::Record* record = pool.enter();
    std::shared_ptr<int> payload(new int(42));
    std::weak_ptr<int> weak(payload);
    std::vector<Arena::Node*> nodes;
    for (size_t i = 0; ok && i < 2000; ++i)
    {
        LockFreeQueueResult status;
        Arena::Node* node = pool.acquireNode(*record, status);
        ok = node != nullptr && status == LockFreeQueueSuccess;
        if (ok)
        {
            node->construct(payload);
            nodes.push_back(node);
        }
    }
    for (size_t i = 0; i < nodes.size(); i += 2)
    {
        pool.releaseNode(nodes[i]);
    }
    // Leave private cached Nodes, live payloads, and unconstructed free storage
    // together. Quiescent destroy must release exactly the remaining payloads.
    pool.leave(*record, true);
    payload.reset();
    ok = !weak.expired() && ok;
    pool.destroy();
    ok = weak.expired() && ok;
    pool.destroy();
    std::cout << "ARENA lazy-lifetime nodes=" << nodes.size() << " valid=" << ok << std::endl;
    return ok;
}

/** 耗尽辅助登记上限、复用无额外申请并验证实际字节释放
@return 全部独立预期是否成立
*/
static bool LockFreeQueueArenaAuxiliaryBound()
{
    typedef LockFreeQueueArena<uint64_t> Arena;
    Arena pool;
    LockFreeQueuePoolConfig config;
    config.initialCapacity = 1;
    config.maximumCapacity = 1;
    config.maximumProducers = 3;
    bool ok = pool.configure(config) == LockFreeQueueSuccess && pool.initialize() == LockFreeQueueSuccess;
    Arena::Record* record = pool.enter();
    std::vector<Arena::Segment*> segments;
    const size_t expected = 3 + 4 * config.maximumProducers + 4;
    for (size_t i = 0; ok && i < expected; ++i)
    {
        LockFreeQueueResult result;
        Arena::Segment* segment = pool.acquireSegment(*record, false, &result);
        ok = segment != nullptr && result == LockFreeQueueSuccess;
        if (ok)
        {
            segments.push_back(segment);
        }
    }
    LockFreeQueueResult refused;
    ok = pool.acquireSegment(*record, false, &refused) == nullptr && refused == LockFreeQueueCapacityLimit && ok;
    const auto full = pool.statistics();
    for (size_t i = 0; i < segments.size(); ++i)
    {
        pool.releaseSegment(segments[i], record);
    }
    segments.clear();
    for (size_t i = 0; ok && i < expected; ++i)
    {
        Arena::Segment* segment = pool.acquireSegment(*record);
        ok = segment != nullptr;
        if (ok)
        {
            segments.push_back(segment);
        }
    }
    ok = pool.statistics().allocationCalls == full.allocationCalls && ok;
    for (size_t i = 0; i < segments.size(); ++i)
    {
        pool.releaseSegment(segments[i], record);
    }
    pool.leave(*record, true);
    pool.setShrinkPolicy(
        [](const LockFreeQueueShrinkContext&)
        {
            LockFreeQueueShrinkDecision result = {1, 1};
            return result;
        });
    for (size_t i = 0; i < 1024; ++i)
    {
        pool.maintain(1);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(4));
    for (size_t i = 0; i < 1024; ++i)
    {
        pool.maintain(1);
    }
    const auto shrunk = pool.statistics();
    const size_t releasedBytes = full.retainedBytes - shrunk.retainedBytes;
    ok = releasedBytes == (expected - 3) * sizeof(Arena::Segment) && shrunk.currentCapacity == 1 &&
         shrunk.peakReservedCapacity == 1 && ok;
    std::cout << "ARENA auxiliary slots=" << expected - 3 << " released_bytes=" << releasedBytes << " valid=" << ok
              << std::endl;
    return ok;
}

/** 旧遍历结束后另一个生产者发布增长块时，不能误报容量满
@return 两个发布项均接纳且完整 FIFO 是否成立
*/
static bool LockFreeQueueGrowthPublicationRace()
{
    LockFreeQueue<uint64_t> queue;
    queue.destroy();
    LockFreeQueuePoolConfig config;
    config.initialCapacity = 64;
    config.maximumCapacity = 128;
    config.maximumProducers = 4;
    LFQ_CHECK(queue.configurePool(config) == LockFreeQueueSuccess && queue.tryInit() == LockFreeQueueSuccess);
    for (uint64_t i = 0; i < 63; ++i)
    {
        LFQ_CHECK(queue.tryPush(i) == LockFreeQueueSuccess);
    }
    LockFreeQueueResult result = LockFreeQueueTemporaryUnavailable;
    LockFreeQueueTestControl::m_pausePoint.store(18);
    std::atomic<bool> stop(false);
    LockFreeQueueThreadGroup group(stop, &LockFreeQueueTestControl::m_release);
    group.m_threads.reserve(1);
    group.m_threads.push_back(std::thread(
        [&]
        {
            g_lockFreeQueueTestLane = 1;
            result = queue.tryPush(64);
        }));
    const bool reached = LockFreeQueueTestControl::waitReached();
    const LockFreeQueueResult mainResult = queue.tryPush(63);
    LockFreeQueueTestControl::m_release.store(true);
    group.join();
    LFQ_CHECK(reached && mainResult == LockFreeQueueSuccess && result == LockFreeQueueSuccess);
    LFQ_CHECK(queue.poolStatistics().currentCapacity == 128);
    uint64_t output = UINT64_MAX;
    for (uint64_t i = 0; i < 65; ++i)
    {
        LFQ_CHECK(queue.pop(&output) && output == i);
    }
    LFQ_CHECK(queue.empty());
    LockFreeQueueTestControl::reset();
    return true;
}

#endif