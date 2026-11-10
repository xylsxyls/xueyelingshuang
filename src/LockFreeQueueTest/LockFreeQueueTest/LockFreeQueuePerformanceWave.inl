inline bool LockFreeQueuePerformanceWave::scenario(size_t producers, size_t threshold)
{
    LockFreeQueue<uint64_t> queue;
#ifndef LOCK_FREE_QUEUE_BASELINE
    queue.destroy();
    LockFreeQueuePoolConfig config;
    config.initialCapacity = 750;
    config.maximumCapacity = 1500;
    config.maximumProducers = 32;
    config.growThreshold = threshold;
    if (queue.configurePool(config) != LockFreeQueueSuccess ||
        queue.setGrowPolicy([](const LockFreeQueueGrowContext& context) { (void)context; return size_t(128); }) != LockFreeQueueSuccess ||
        queue.setShrinkPolicy([](const LockFreeQueueShrinkContext& context) {
            LockFreeQueueShrinkDecision decision = {
                context.currentCapacity > context.initialCapacity ? context.currentCapacity - context.initialCapacity : 0, 10};
            return decision;
        }) != LockFreeQueueSuccess || queue.tryInit() != LockFreeQueueSuccess)
    {
        return false;
    }
#endif
    for (uint64_t i = 0; i < 500; ++i)
    {
        LockFreeQueueBenchmarkPush::checked(queue, i);
    }
    const size_t cycles = 4;
    const size_t counts[3] = {500, 32768, 262144};
    const size_t stages = cycles * 3;
    std::atomic<bool> start(false), stop(false), failed(false);
    std::atomic<size_t> ready(0), epoch(0), completed(0), issued(500), committed(500), consumed(0);
    std::vector<uint64_t> expected(producers, 0), sent(producers, 0), rejected(producers, 0);
    std::vector<std::vector<uint64_t>> pushSamples(producers);
    std::vector<uint64_t> popSamples;
    std::vector<size_t> capacities(stages, 0), grows(stages, 0), live(stages, 0);
    popSamples.reserve(20000);
    LockFreeQueueThreadGroup group(stop, &start);
    group.m_threads.reserve(producers);
    for (size_t lane = 0; lane < producers; ++lane)
    {
        pushSamples[lane].reserve(20000 / producers + 64);
        group.m_threads.emplace_back([&, lane]() {
            try
            {
                uint64_t sequence = lane == 0 ? 500 : 0;
                ready.fetch_add(1);
                while (!start.load() && !stop.load())
                {
                    std::this_thread::yield();
                }
                for (size_t stage = 0; stage < stages && !stop.load(); ++stage)
                {
                    while (epoch.load(std::memory_order_acquire) != stage + 1 && !stop.load())
                    {
                        std::this_thread::yield();
                    }
                    const size_t count = counts[stage % 3] / producers + (lane < counts[stage % 3] % producers ? 1 : 0);
                    const size_t window = stage % 3 == 2 ? 600 : 1000;
                    for (size_t i = 0; i < count && !stop.load(); ++i)
                    {
                        // Test-driver traffic shaping only: these credits are
                        // outside the queue, identical for every implementation.
                        const size_t ticket = issued.fetch_add(1, std::memory_order_relaxed);
                        while (ticket >= consumed.load(std::memory_order_acquire) + window && !stop.load())
                        {
                            std::this_thread::yield();
                        }
                        if (stop.load())
                        {
                            break;
                        }
                        const uint64_t stamp = i % 64 == 0 ? LockFreeQueueMeasurements::ticks() : 0;
                        for (;;)
                        {
                            const int32_t result = LockFreeQueueBenchmarkPush::submit(queue, (uint64_t(lane) << 48) | sequence);
                            if (result == 0)
                            {
                                break;
                            }
#ifndef LOCK_FREE_QUEUE_BASELINE
                            if (!LockFreeQueueBenchmarkPush::transient(result) && result != LockFreeQueueCapacityLimit)
                            {
                                throw LockFreeQueueError(static_cast<LockFreeQueueResult>(result));
                            }
#else
                            throw std::runtime_error("Baseline push failed");
#endif
                            ++rejected[lane];
                            if (stop.load())
                            {
                                return;
                            }
                            std::this_thread::yield();
                        }
                        if (i % 64 == 0)
                        {
                            pushSamples[lane].push_back(LockFreeQueueMeasurements::ticks() - stamp);
                        }
                        ++sequence;
                        committed.fetch_add(1, std::memory_order_release);
                    }
                    completed.fetch_add(1, std::memory_order_release);
                }
                sent[lane] = sequence;
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
    size_t received = 0, released = 0, maintenanceCalls = 0, capacityPeak = 0;
    uint64_t maintenanceNs = 0;
    std::cout << "WAVE case=wave750_t" << threshold << "_p" << producers << " initial=750 peak_items=1000 valley_items=500"
        " maximum=1500 grow_step=128 threshold=" << threshold << " idle_ms=10 owner_interval_ms=1 owner_budget=64 cycles=" << cycles << std::endl;
    LockFreeQueueMeasurements::resources("wave_before");
    g_lockFreeQueueMeasureCalls.store(0);
    g_lockFreeQueueMeasureBytes.store(0);
    const uint64_t cpuStart = LockFreeQueueMeasurements::cpu();
    const uint64_t began = LockFreeQueueMeasurements::ticks();
    uint64_t lastMaintenance = began;
    g_lockFreeQueueMeasureAllocations.store(!g_lockFreeQueueMeasureSkipAllocations);
    start.store(true);
    for (size_t stage = 0; stage < stages && valid && !stop.load(); ++stage)
    {
        completed.store(0);
        epoch.store(stage + 1, std::memory_order_release);
        size_t polls = 0;
        while (!stop.load())
        {
            if (stage % 3 != 0 && committed.load(std::memory_order_acquire) > received + 500)
            {
                uint64_t value = UINT64_MAX;
                const uint64_t stamp = received % 64 == 0 ? LockFreeQueueMeasurements::ticks() : 0;
                if (queue.pop(&value))
                {
                    const size_t lane = static_cast<size_t>(value >> 48);
                    valid = lane < producers && (value & 0x0000ffffffffffffULL) == expected[lane]++ && valid;
                    if (received % 64 == 0)
                    {
                        popSamples.push_back(LockFreeQueueMeasurements::ticks() - stamp);
                    }
                    consumed.store(++received, std::memory_order_release);
                }
            }
            else if (completed.load(std::memory_order_acquire) == producers)
            {
                // A last commit may race the first committed load above. The
                // completion acquire orders all producer commits; recheck the
                // drain target after that synchronization before ending a phase.
                if (stage % 3 == 0 || committed.load(std::memory_order_acquire) == received + 500)
                {
                    break;
                }
            }
            else
            {
                std::this_thread::yield();
            }
            if ((++polls & 255) == 0)
            {
                const uint64_t time = LockFreeQueueMeasurements::ticks();
                if (!valid || time - began > 60000000000ULL)
                {
                    valid = false;
                    stop.store(true);
                }
#ifndef LOCK_FREE_QUEUE_BASELINE
                if (time - lastMaintenance >= 1000000)
                {
                    const LockFreeQueueMaintenanceResult result = queue.maintain(64);
                    maintenanceNs += LockFreeQueueMeasurements::ticks() - time;
                    lastMaintenance = time;
                    ++maintenanceCalls;
                    released += result.releasedCapacity;
                    valid = result.result == LockFreeQueueSuccess && valid;
                }
#endif
            }
        }
        live[stage] = committed.load() - received;
        valid = live[stage] == (stage % 3 == 0 ? 1000 : 500) && valid;
#ifndef LOCK_FREE_QUEUE_BASELINE
        const LockFreeQueuePoolStatistics state = queue.poolStatistics();
        capacities[stage] = state.currentCapacity;
        grows[stage] = static_cast<size_t>(state.successfulGrowCount);
        capacityPeak = (std::max)(capacityPeak, state.peakReservedCapacity);
        valid = state.currentCapacity >= 750 && state.peakReservedCapacity <= 1500 && valid;
#endif
    }
    stop.store(true);
    group.join();
    while (valid && received < committed.load())
    {
        uint64_t value = UINT64_MAX;
        if (!queue.pop(&value))
        {
            valid = false;
            break;
        }
        const size_t lane = static_cast<size_t>(value >> 48);
        valid = lane < producers && (value & 0x0000ffffffffffffULL) == expected[lane]++ && valid;
        ++received;
    }
    const uint64_t elapsed = LockFreeQueueMeasurements::ticks() - began;
    g_lockFreeQueueMeasureAllocations.store(false);
    const uint64_t cpuElapsed = LockFreeQueueMeasurements::cpu() - cpuStart;
    valid = !failed.load() && received == 500 + cycles * (counts[0] + counts[1] + counts[2]) && queue.empty() && valid;
    std::vector<uint64_t> combined;
    size_t retries = 0;
    for (size_t lane = 0; lane < producers; ++lane)
    {
        valid = sent[lane] == expected[lane] && valid;
        retries += rejected[lane];
        combined.insert(combined.end(), pushSamples[lane].begin(), pushSamples[lane].end());
    }
#ifndef LOCK_FREE_QUEUE_BASELINE
    const LockFreeQueuePoolStatistics state = queue.poolStatistics();
    valid = state.successfulGrowCount > 0 && state.activeProducers == 0 && valid;
#endif
    for (size_t stage = 0; stage < stages; ++stage)
    {
        std::cout << "WAVE_STAGE cycle=" << stage / 3 << " phase=" << stage % 3 << " backlog=" << live[stage]
            << " capacity=" << capacities[stage] << " grows=" << grows[stage] << std::endl;
    }
    std::cout << "WAVE_RESULT operations=" << received << " elapsed_ns=" << elapsed << " cpu_ns=" << cpuElapsed
        << " allocations=" << g_lockFreeQueueMeasureCalls.load() << " allocated_bytes=" << g_lockFreeQueueMeasureBytes.load()
        << " rejected_retries=" << retries << " explicit_maintenance_calls=" << maintenanceCalls << " explicit_maintenance_ns=" << maintenanceNs
        << " explicit_released=" << released << " peak_capacity=" << capacityPeak << " valid=" << valid << std::endl;
    LockFreeQueueMeasurements::latency("push", combined);
    LockFreeQueueMeasurements::latency("pop", popSamples);
    LockFreeQueueMeasurements::resources("wave_after");
    return valid;
}

inline bool LockFreeQueuePerformanceWave::run()
{
    bool valid = true;
    for (size_t threshold = 0; threshold <= 8; threshold += 8)
    {
        for (size_t producers = 1; producers <= 16; producers *= 2)
        {
            valid = scenario(producers, threshold) && valid;
        }
    }
    std::cout << (valid ? "PASS" : "FAIL") << " id=40" << std::endl;
    return valid;
}
