inline int32_t LockFreeQueuePerformancePools::submit(LockFreeQueue<uint64_t>& queue,
    uint64_t value, LockFreeQueueProducer<uint64_t>* producer)
{
#ifdef LOCK_FREE_QUEUE_BASELINE
    (void)producer;
    LockFreeQueueBenchmarkPush::checked(queue, value);
    return 0;
#else
#if LOCK_FREE_QUEUE_BENCH_VARIANT == 2
    (void)producer;
    return queue.tryPush(value);
#elif LOCK_FREE_QUEUE_BENCH_VARIANT == 3
    return producer->tryPush(value);
#else
#if LOCK_FREE_QUEUE_BENCH_VARIANT == 4
    return LockFreeQueueBenchmarkPush::submit(*producer, value);
#else
    (void)producer;
    return LockFreeQueueBenchmarkPush::submit(queue, value);
#endif
#endif
#endif
}

inline void LockFreeQueuePerformancePools::pool(const LockFreeQueue<uint64_t>& queue)
{
#ifndef LOCK_FREE_QUEUE_BASELINE
    const LockFreeQueuePoolStatistics s = queue.poolStatistics();
    std::cout << "POLICY_POOL capacity=" << s.currentCapacity << " used=" << s.usedNodes <<
        " free=" << s.reusableFreeCapacity << " retired=" << s.retiredNodes <<
        " pending=" << s.retiringBlockCapacity << " reserved=" << s.reservedCapacity <<
        " peak=" << s.peakReservedCapacity << " retained_bytes=" << s.retainedBytes <<
        " allocations=" << s.allocationCalls << " grows=" << s.successfulGrowCount <<
        " retries=" << s.casRetries << " active=" << s.activeProducers << std::endl;
#else
    (void)queue;
#endif
}

inline bool LockFreeQueuePerformancePools::cycles(const char* name, size_t threshold,
    size_t growth, bool shrink)
{
    std::unique_ptr<LockFreeQueue<uint64_t>> instance(new LockFreeQueue<uint64_t>());
    LockFreeQueue<uint64_t>& queue = *instance;
    const size_t count = growth == 0 ? 32768 : 4095;
    const size_t rounds = shrink ? 8 : 4;
    bool valid = true;
#ifndef LOCK_FREE_QUEUE_BASELINE
    queue.destroy();
    LockFreeQueuePoolConfig config;
    config.initialCapacity = 64;
    config.maximumCapacity = growth == 0 ? -1 : 4096;
    config.growThreshold = threshold;
    if (queue.configurePool(config) != LockFreeQueueSuccess || queue.tryInit() != LockFreeQueueSuccess)
    {
        return false;
    }
    if (growth != 0)
    {
        // Snapshot-only policy; no mutation of the pool from inside its callback.
        valid = queue.setGrowPolicy([growth](const LockFreeQueueGrowContext& c) {
            return growth == 2 ? (std::max)(c.initialCapacity, c.currentCapacity / 2) :
                (growth == 3 ? size_t(512) : size_t(256));
        }) == LockFreeQueueSuccess && valid;
    }

#endif
    for (size_t round = 0; round < rounds; ++round)
    {
#ifndef LOCK_FREE_QUEUE_BASELINE
        if (shrink)
        {
            // Each cycle times growth with shrinking disabled; explicit owner
            // maintenance below is the separately timed physical shrink phase.
            valid = queue.setShrinkPolicy(LockFreeQueueShrinkPolicy()) == LockFreeQueueSuccess && valid;
        }
#endif
        LockFreeQueueProducer<uint64_t>* session = nullptr;
#if LOCK_FREE_QUEUE_BENCH_VARIANT >= 3
        LockFreeQueueProducer<uint64_t> owner;
        if (queue.tryAcquireProducer(owner) != LockFreeQueueSuccess)
        {
            return false;
        }
        session = &owner;
#endif
        std::vector<uint64_t> pushes, pops;
        pushes.reserve((count + 63) / 64);
        pops.reserve((count + 63) / 64);
#ifndef LOCK_FREE_QUEUE_BASELINE
        const LockFreeQueuePoolStatistics before = queue.poolStatistics();
#endif
        std::cout << "POLICY case=" << name << " phase=" << (round == 0 || shrink ? "growth" : "reuse") <<
            " cycle=" << round << " threshold=" << threshold << " growth_policy=" << growth <<
            " finite_limit=" << (growth == 0 ? -1 : 4096) << " items=" << count << std::endl;
        g_lockFreeQueueMeasureCalls.store(0);
        g_lockFreeQueueMeasureBytes.store(0);
        const uint64_t cpuStart = LockFreeQueueMeasurements::cpu();
        const uint64_t started = LockFreeQueueMeasurements::ticks();
        g_lockFreeQueueMeasureAllocations.store(!g_lockFreeQueueMeasureSkipAllocations);
        for (size_t i = 0; i < count; ++i)
        {
            const uint64_t stamp = i % 64 == 0 ? LockFreeQueueMeasurements::ticks() : 0;
            if (submit(queue, i + round * count, session) != 0)
            {
                valid = false;
                break;
            }
            if (i % 64 == 0)
            {
                pushes.push_back(LockFreeQueueMeasurements::ticks() - stamp);
            }
        }
        for (size_t i = 0; i < count; ++i)
        {
            uint64_t value = UINT64_MAX;
            const uint64_t stamp = i % 64 == 0 ? LockFreeQueueMeasurements::ticks() : 0;
            valid = queue.pop(&value) && value == i + round * count && valid;
            if (i % 64 == 0)
            {
                pops.push_back(LockFreeQueueMeasurements::ticks() - stamp);
            }
        }
        const uint64_t elapsed = LockFreeQueueMeasurements::ticks() - started;
        g_lockFreeQueueMeasureAllocations.store(false);
        const uint64_t cpuElapsed = LockFreeQueueMeasurements::cpu() - cpuStart;
        const uint64_t allocations = g_lockFreeQueueMeasureCalls.load();
        const uint64_t bytes = g_lockFreeQueueMeasureBytes.load();
#if LOCK_FREE_QUEUE_BENCH_VARIANT >= 3
        owner.release();
#endif
        valid = queue.empty() && valid;
#ifndef LOCK_FREE_QUEUE_BASELINE
        LockFreeQueuePoolStatistics after = queue.poolStatistics();
        valid = after.activeProducers == 0 && (growth == 0 || after.peakReservedCapacity <= 4096) && valid;
        valid = (round == 0 || shrink ? after.successfulGrowCount > before.successfulGrowCount :
            after.allocationCalls == before.allocationCalls) && valid;
#endif
        std::cout << "POLICY_RESULT operations=" << count << " elapsed_ns=" << elapsed <<
            " items_per_second=" << count * 1e9 / elapsed << " cpu_ns=" << cpuElapsed <<
            " allocations=" << allocations << " allocated_bytes=" << bytes << " valid=" << valid << std::endl;
        LockFreeQueueMeasurements::latency("push", pushes);
        LockFreeQueueMeasurements::latency("pop", pops);
        pool(queue);
        LockFreeQueueMeasurements::resources("policy_drained");
#ifndef LOCK_FREE_QUEUE_BASELINE
        // Cleanup is outside the roundtrip timer and measured separately.
        const uint64_t maintenanceStarted = LockFreeQueueMeasurements::ticks();
        for (size_t pass = 0; pass < 8; ++pass)
        {
            valid = queue.maintain(count * 4 + 1024).result == LockFreeQueueSuccess && valid;
        }
        const uint64_t cleanupNs = LockFreeQueueMeasurements::ticks() - maintenanceStarted;
        after = queue.poolStatistics();
        valid = after.retiredNodes == 0 && after.usedNodes == 1 && valid;
        std::cout << "MAINTENANCE phase=reclaim elapsed_ns=" << cleanupNs << " valid=" << valid << std::endl;
        if (shrink)
        {
            // A positive idle duration must elapse before actual block release.
            valid = queue.setShrinkPolicy([](const LockFreeQueueShrinkContext& c) {
                LockFreeQueueShrinkDecision d = { c.currentCapacity, c.currentCapacity > 1024 ? uint64_t(2) : uint64_t(1) };
                return d;
            }) == LockFreeQueueSuccess && valid;
        }
        if (shrink)
        {
            // No test clock: wait outside timing, then measure actual detach/free.
            const size_t beforeCapacity = after.currentCapacity;
            std::this_thread::sleep_for(std::chrono::milliseconds(4));
            const uint64_t shrinkStarted = LockFreeQueueMeasurements::ticks();
            size_t released = 0;
            for (size_t pass = 0; pass < 8; ++pass)
            {
                const LockFreeQueueMaintenanceResult r = queue.maintain(65536);
                released += r.releasedCapacity;
                valid = r.result == LockFreeQueueSuccess && valid;
            }
            const uint64_t shrinkNs = LockFreeQueueMeasurements::ticks() - shrinkStarted;
            after = queue.poolStatistics();
            valid = released > 0 && after.currentCapacity < beforeCapacity &&
                after.currentCapacity >= 64 && after.currentCapacity <= 576 && valid;
            std::cout << "MAINTENANCE phase=shrink elapsed_ns=" << shrinkNs <<
                " released=" << released << " before_capacity=" << beforeCapacity <<
                " after_capacity=" << after.currentCapacity << " valid=" << valid << std::endl;
            pool(queue);
            LockFreeQueueMeasurements::resources("policy_after_shrink");
        }
#else
        if (shrink)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(4));
            std::cout << "MAINTENANCE NOT_APPLICABLE=original_has_no_pool" << std::endl;
        }
#endif
        if (!valid)
        {
            return false;
        }
    }
    return valid;
}

inline bool LockFreeQueuePerformancePools::pressure(size_t threshold, size_t producers)
{
    std::unique_ptr<LockFreeQueue<uint64_t>> instance(new LockFreeQueue<uint64_t>());
    LockFreeQueue<uint64_t>& queue = *instance;
    const size_t count = 200000 / producers;
#ifndef LOCK_FREE_QUEUE_BASELINE
    queue.destroy();
    LockFreeQueuePoolConfig config;
    config.initialCapacity = 256;
    config.maximumCapacity = 256;
    config.growThreshold = threshold;
    if (queue.configurePool(config) != LockFreeQueueSuccess || queue.tryInit() != LockFreeQueueSuccess)
    {
        return false;
    }
#endif
#ifndef LOCK_FREE_QUEUE_BASELINE
    const LockFreeQueuePoolStatistics initialized = queue.poolStatistics();
#endif
    std::cout << "POLICY case=bounded" << threshold << "_p" << producers << " phase=pressure cycle=0 items=" << count * producers << std::endl;
    std::atomic<bool> start(false), stop(false), failed(false);
    std::atomic<size_t> ready(0);
    std::vector<uint64_t> next(producers, 0), refused(producers, 0);
    std::vector<std::vector<uint64_t>> samples(producers);
    LockFreeQueueThreadGroup group(stop, &start);
    group.m_threads.reserve(producers);
    for (size_t lane = 0; lane < producers; ++lane)
    {
        samples[lane].reserve((count + 63) / 64);
        // Each worker owns its sequence, rejection count and latency vector.
        group.m_threads.emplace_back([&, lane]() {
            try
            {
                LockFreeQueueProducer<uint64_t>* session = nullptr;
#if LOCK_FREE_QUEUE_BENCH_VARIANT >= 3
                LockFreeQueueProducer<uint64_t> owner;
                if (queue.tryAcquireProducer(owner) != LockFreeQueueSuccess)
                {
                    failed.store(true);
                    stop.store(true);
                }
                session = &owner;
#endif
                ready.fetch_add(1);
                while (!start.load() && !stop.load())
                {
                    std::this_thread::yield();
                }
                for (size_t index = 0; index < count && !stop.load(); ++index)
                {
                    const uint64_t stamp = index % 64 == 0 ? LockFreeQueueMeasurements::ticks() : 0;
                    for (;;)
                    {
                        const int32_t result = submit(queue, (uint64_t(lane) << 32) | index, session);
                        if (result == 0)
                        {
                            break;
                        }
                        ++refused[lane];
#ifndef LOCK_FREE_QUEUE_BASELINE
                        if (result != LockFreeQueueCapacityLimit && !LockFreeQueueBenchmarkPush::transient(result))
                        {
                            failed.store(true);
                            stop.store(true);
                        }
#endif
                        if (stop.load())
                        {
                            break;
                        }
                        std::this_thread::yield();
                    }
                    if (index % 64 == 0)
                    {
                        samples[lane].push_back(LockFreeQueueMeasurements::ticks() - stamp);
                    }
                }
            }
            catch (...)
            {
                failed.store(true);
                stop.store(true);
            }
        });
    }
    const uint64_t readyDeadline = LockFreeQueueMeasurements::ticks() + 5000000000ULL;
    while (ready.load() != producers && !stop.load() && LockFreeQueueMeasurements::ticks() < readyDeadline)
    {
        std::this_thread::yield();
    }
    bool valid = ready.load() == producers && !stop.load();
    size_t received = 0;
    g_lockFreeQueueMeasureCalls.store(0);
    g_lockFreeQueueMeasureBytes.store(0);
    const uint64_t cpuStart = LockFreeQueueMeasurements::cpu();
    const uint64_t began = LockFreeQueueMeasurements::ticks();
    g_lockFreeQueueMeasureAllocations.store(!g_lockFreeQueueMeasureSkipAllocations);
    start.store(true);
    while (valid && received < count * producers && !stop.load() && LockFreeQueueMeasurements::ticks() - began < 30000000000ULL)
    {
        uint64_t value = 0;
        if (queue.pop(&value))
        {
            const size_t lane = static_cast<size_t>(value >> 32);
            valid = lane < producers && uint32_t(value) == next[lane]++ && valid;
            ++received;
        }
        else
        {
            std::this_thread::yield();
        }
    }
    stop.store(true);
    group.join();
    const uint64_t elapsed = LockFreeQueueMeasurements::ticks() - began;
    g_lockFreeQueueMeasureAllocations.store(false);
    const uint64_t cpuElapsed = LockFreeQueueMeasurements::cpu() - cpuStart;
    valid = valid && !failed.load() && received == count * producers && queue.empty();
    uint64_t rejections = 0;
    std::vector<uint64_t> combined;
    for (size_t lane = 0; lane < producers; ++lane)
    {
        valid = next[lane] == count && valid;
        rejections += refused[lane];
        combined.insert(combined.end(), samples[lane].begin(), samples[lane].end());
    }
#ifndef LOCK_FREE_QUEUE_BASELINE
    const LockFreeQueuePoolStatistics after = queue.poolStatistics();
    // The new allocator has two initial allocations. Extra publication segments
    // are a bounded metadata cache, not extra payload capacity. Keep measuring
    // their actual allocations inside the workload; do not prewarm them away.
    const size_t auxiliaryLimit = config.maximumProducers * 4 + 4;
    valid = after.currentCapacity == 256 && after.peakReservedCapacity == 256 && after.activeProducers == 0 &&
        after.successfulGrowCount == 0 && initialized.allocationCalls == 2 &&
        after.allocationCalls >= initialized.allocationCalls &&
        after.allocationCalls - initialized.allocationCalls <= auxiliaryLimit &&
        after.retainedBytes == initialized.retainedBytes +
            (after.allocationCalls - initialized.allocationCalls) * sizeof(LockFreeQueueArena<uint64_t>::Segment) && valid;
    // Preserve the exact empty-pool oracle after explicitly returning private
    // batches. This cleanup is outside timing, and never changes accepted data.
    const uint64_t cleanupStarted = LockFreeQueueMeasurements::ticks();
    valid = queue.maintain(config.maximumProducers * 8 + 1024).result == LockFreeQueueSuccess && valid;
    const uint64_t cleanupNs = LockFreeQueueMeasurements::ticks() - cleanupStarted;
    std::cout << "MAINTENANCE phase=reclaim elapsed_ns=" << cleanupNs << " valid=" << valid << std::endl;
    const LockFreeQueuePoolStatistics reclaimed = queue.poolStatistics();
    valid = reclaimed.usedNodes == 1 && reclaimed.reusableFreeCapacity == 255 &&
        reclaimed.retiredNodes == 0 && reclaimed.reservedCapacity == 0 && valid;
#endif
    std::cout << "POLICY_RESULT operations=" << received << " elapsed_ns=" << elapsed <<
        " items_per_second=" << received * 1e9 / elapsed << " cpu_ns=" << cpuElapsed <<
        " allocations=" << g_lockFreeQueueMeasureCalls.load() << " allocated_bytes=" << g_lockFreeQueueMeasureBytes.load() <<
        " rejected_retries=" << rejections << " valid=" << valid << std::endl;
    LockFreeQueueMeasurements::latency("push_including_retry", combined);
    pool(queue);
    LockFreeQueueMeasurements::resources("bounded_after_join");
    return valid;
}

inline bool LockFreeQueuePerformancePools::run()
{
    bool valid = cycles("default_zero", 0, 0, false);
    valid = cycles("default_positive", 8, 0, false) && valid;
    valid = cycles("finite_fixed_zero", 0, 1, false) && valid;
    valid = cycles("finite_fixed_positive", 8, 1, false) && valid;
    valid = cycles("finite_proportional", 8, 2, false) && valid;
    valid = cycles("shrink_regrow_zero", 0, 3, true) && valid;
    valid = cycles("shrink_regrow_positive", 8, 3, true) && valid;
    for (size_t p = 1; p <= 16; p *= 4)
    {
        valid = pressure(0, p) && valid;
        valid = pressure(1, p) && valid;
    }
    std::cout << (valid ? "PASS" : "FAIL") << " id=38" << std::endl;
    return valid;
}
