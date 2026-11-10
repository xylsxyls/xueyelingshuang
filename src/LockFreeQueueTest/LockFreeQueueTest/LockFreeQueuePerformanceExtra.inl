inline uint64_t LockFreeQueuePerformanceExtra::begin(uint64_t& cpuStart)
{
    g_lockFreeQueueMeasureCalls.store(0);
    g_lockFreeQueueMeasureBytes.store(0);
    cpuStart = LockFreeQueueMeasurements::cpu();
    g_lockFreeQueueMeasureAllocations.store(!g_lockFreeQueueMeasureSkipAllocations);
    return LockFreeQueueMeasurements::ticks();
}

inline void LockFreeQueuePerformanceExtra::finish(const char* operation, uint64_t began,
    uint64_t cpuStart, size_t count, bool valid)
{
    const uint64_t elapsed = LockFreeQueueMeasurements::ticks() - began;
    g_lockFreeQueueMeasureAllocations.store(false);
    const uint64_t cpuElapsed = LockFreeQueueMeasurements::cpu() - cpuStart;
    std::cout << "EXTRA operation=" << operation << " operations=" << count << " elapsed_ns=" << elapsed <<
        " ns_per_operation=" << double(elapsed) / count << " cpu_ns=" << cpuElapsed <<
        " allocations=" << g_lockFreeQueueMeasureCalls.load() << " allocated_bytes=" << g_lockFreeQueueMeasureBytes.load() <<
        " valid=" << valid << std::endl;
}

inline bool LockFreeQueuePerformanceExtra::observe(size_t recordCount)
{
    std::unique_ptr<LockFreeQueue<uint64_t>> instance(new LockFreeQueue<uint64_t>());
    LockFreeQueue<uint64_t>& queue = *instance;
    uint64_t allocationBaseline = 0;
    if (!LockFreeQueueMeasurements::preparePool(queue, 63, allocationBaseline))
    {
        return false;
    }
#ifndef LOCK_FREE_QUEUE_BASELINE
    std::vector<std::unique_ptr<LockFreeQueueProducer<uint64_t>>> sessions(recordCount);
    for (size_t i = 0; i < recordCount; ++i)
    {
        sessions[i].reset(new LockFreeQueueProducer<uint64_t>());
        if (queue.tryAcquireProducer(*sessions[i]) != LockFreeQueueSuccess)
        {
            return false;
        }
    }
    sessions.clear();
#endif
    std::cout << "OBSERVER record_high_water=" << recordCount << std::endl;
    const size_t count = 1000000;
    volatile uint64_t sum = 0;
    uint64_t cpuStart = 0;
    uint64_t began = begin(cpuStart);
    for (size_t i = 0; i < count; ++i)
    {
        sum += queue.size();
    }
    bool valid = sum == 0;
    finish("size_empty", began, cpuStart, count, valid);
    began = begin(cpuStart);
    for (size_t i = 0; i < count; ++i)
    {
        sum += queue.empty() ? 1 : 0;
    }
    valid = sum == count && valid;
    finish("empty_empty", began, cpuStart, count, valid);
    sum = 0;
    uint64_t value = UINT64_MAX;
    began = begin(cpuStart);
    for (size_t i = 0; i < count; ++i)
    {
        sum += queue.pop(&value) ? 1 : 0;
    }
    valid = sum == 0 && value == UINT64_MAX && valid;
    finish("pop_empty", began, cpuStart, count, valid);
    LockFreeQueueBenchmarkPush::checked(queue, 42);
    began = begin(cpuStart);
    for (size_t i = 0; i < count; ++i)
    {
        sum += queue.size();
    }
    valid = sum == count && valid;
    finish("size_one", began, cpuStart, count, valid);
    valid = queue.pop(&value) && value == 42 && queue.empty() && valid;

    // A recently active queue can have a different empty path from a never-used
    // queue. Charge the whole push/pop/empty cycle and also time the empty call.
    const size_t cycles = 10000;
    std::vector<uint64_t> emptyLatencies, emptyBurstLatencies;
    emptyLatencies.reserve(cycles);
    emptyBurstLatencies.reserve(cycles * 16);
    began = begin(cpuStart);
    for (size_t i = 0; i < cycles; ++i)
    {
        LockFreeQueueBenchmarkPush::checked(queue, i);
        valid = queue.pop(&value) && value == i && valid;
        for (size_t probe = 0; probe < 16; ++probe)
        {
            const uint64_t emptyStarted = LockFreeQueueMeasurements::ticks();
            const bool popped = queue.pop(&value);
            const uint64_t elapsed = LockFreeQueueMeasurements::ticks() - emptyStarted;
            if (probe == 0)
            {
                emptyLatencies.push_back(elapsed);
            }
            emptyBurstLatencies.push_back(elapsed);
            valid = !popped && value == i && valid;
        }
    }
    valid = queue.empty() && valid;
    finish("push_pop_empty_cycle", began, cpuStart, cycles, valid);
    LockFreeQueueMeasurements::latency("pop_empty_after_activity", emptyLatencies);
    LockFreeQueueMeasurements::latency("pop_empty_poll_burst16", emptyBurstLatencies);
    return valid;
}

inline bool LockFreeQueuePerformanceExtra::lifecycle()
{
    bool valid = true;
    const size_t repeats = 2000;
    uint64_t cpuStart = 0;
    uint64_t began = begin(cpuStart);
    for (size_t i = 0; i < repeats; ++i)
    {
        std::unique_ptr<LockFreeQueue<uint64_t>> queue(new LockFreeQueue<uint64_t>());
        valid = queue->empty() && valid;
    }
    finish("default_construct_destroy", began, cpuStart, repeats, valid);
    std::unique_ptr<LockFreeQueue<uint64_t>> instance(new LockFreeQueue<uint64_t>());
    LockFreeQueue<uint64_t>& queue = *instance;
    uint64_t allocationBaseline = 0;
    if (!LockFreeQueueMeasurements::preparePool(queue, 63, allocationBaseline))
    {
        return false;
    }
    began = begin(cpuStart);
    for (size_t i = 0; i < repeats; ++i)
    {
        queue.destroy();
        queue.init();
        valid = queue.empty() && valid;
    }
    finish("configured_destroy_init", began, cpuStart, repeats, valid);
    // preparePool assumes a fresh default queue for profile 2; this instance is empty and still 64 slots.
    if (!LockFreeQueueMeasurements::preparePool(queue, 65536, allocationBaseline))
    {
        return false;
    }
    for (size_t i = 0; i < 65536; ++i)
    {
        LockFreeQueueBenchmarkPush::checked(queue, i);
    }
    LockFreeQueueMeasurements::resources("before_clear_65536");
    began = begin(cpuStart);
    queue.clear();
    valid = queue.empty() && valid;
    finish("clear_65536", began, cpuStart, 65536, valid);
    LockFreeQueueMeasurements::resources("after_clear_65536");
    for (size_t i = 0; i < 65536; ++i)
    {
        LockFreeQueueBenchmarkPush::checked(queue, i);
    }
    began = begin(cpuStart);
    queue.destroy();
    valid = queue.empty() && valid;
    finish("destroy_65536", began, cpuStart, 65536, valid);
    queue.init();
    valid = queue.empty() && valid;
    return valid;
}

inline bool LockFreeQueuePerformanceExtra::rejection()
{
#ifdef LOCK_FREE_QUEUE_BASELINE
    std::cout << "REJECTION NOT_APPLICABLE=original_has_no_capacity_api" << std::endl;
    return true;
#else
    std::unique_ptr<LockFreeQueue<uint64_t>> instance(new LockFreeQueue<uint64_t>());
    LockFreeQueue<uint64_t>& queue = *instance;
    queue.destroy();
    LockFreeQueuePoolConfig config;
    config.initialCapacity = 2;
    config.maximumCapacity = 2;
    config.growThreshold = g_lockFreeQueueBenchmarkProfile == 0 ? 0 : 1;
    if (queue.configurePool(config) != LockFreeQueueSuccess || queue.tryInit() != LockFreeQueueSuccess)
    {
        return false;
    }
    LockFreeQueueBenchmarkPush::checked(queue, 42);
#if LOCK_FREE_QUEUE_BENCH_VARIANT >= 3
    LockFreeQueueProducer<uint64_t> producer;
    if (queue.tryAcquireProducer(producer) != LockFreeQueueSuccess)
    {
        return false;
    }
#endif
    std::vector<uint64_t> samples;
    const size_t count = 10000;
    samples.reserve(count);
    bool valid = true;
    uint64_t cpuStart = 0;
    const uint64_t began = begin(cpuStart);
    for (size_t i = 0; i < count; ++i)
    {
        const uint64_t start = LockFreeQueueMeasurements::ticks();
        LockFreeQueueResult result = LockFreeQueueSuccess;
#if LOCK_FREE_QUEUE_BENCH_VARIANT == 1
        result = static_cast<LockFreeQueueResult>(LockFreeQueueBenchmarkPush::submit(queue, uint64_t(99)));
#elif LOCK_FREE_QUEUE_BENCH_VARIANT == 2
        result = queue.tryPush(99);
#elif LOCK_FREE_QUEUE_BENCH_VARIANT == 3
        result = producer.tryPush(99);
#else
        result = static_cast<LockFreeQueueResult>(LockFreeQueueBenchmarkPush::submit(producer, uint64_t(99)));
#endif
        samples.push_back(LockFreeQueueMeasurements::ticks() - start);
        valid = result == LockFreeQueueCapacityLimit && valid;
    }
    finish("full_pool_failure", began, cpuStart, count, valid);
    LockFreeQueueMeasurements::latency("full_pool_failure", samples);
#if LOCK_FREE_QUEUE_BENCH_VARIANT >= 3
    producer.release();
#endif
    uint64_t value = 0;
    return queue.pop(&value) && value == 42 && queue.empty() && valid;
#endif
}

inline bool LockFreeQueuePerformanceExtra::registration()
{
#ifdef LOCK_FREE_QUEUE_BASELINE
    std::cout << "REGISTRATION NOT_APPLICABLE=original_has_no_context_api" << std::endl;
    return true;
#else
    std::unique_ptr<LockFreeQueue<uint64_t>> instance(new LockFreeQueue<uint64_t>());
    LockFreeQueue<uint64_t>& queue = *instance;
    uint64_t allocationBaseline = 0;
    if (!LockFreeQueueMeasurements::preparePool(queue, 63, allocationBaseline))
    {
        return false;
    }
    LockFreeQueueProducer<uint64_t> producer;
    bool valid = true;
    const size_t count = 1000000;
    uint64_t cpuStart = 0;
    const uint64_t began = begin(cpuStart);
    for (size_t i = 0; i < count; ++i)
    {
        valid = queue.tryAcquireProducer(producer) == LockFreeQueueSuccess && valid;
        producer.release();
    }
    finish("context_acquire_release", began, cpuStart, count, valid);
    return !producer.attached() && queue.poolStatistics().activeProducers == 0 && valid;
#endif
}

template<class T>
bool LockFreeQueuePerformanceExtra::payload(const char* name, const T& body, size_t count)
{
    typedef LockFreeQueuePerformanceMessage<T> Message;
    std::unique_ptr<LockFreeQueue<Message>> instance(new LockFreeQueue<Message>());
    LockFreeQueue<Message>& queue = *instance;
    const size_t width = 256;
#ifndef LOCK_FREE_QUEUE_BASELINE
    if (g_lockFreeQueueBenchmarkProfile != 2)
    {
        queue.destroy();
        LockFreeQueuePoolConfig config;
        config.initialCapacity = width + 1;
        config.maximumCapacity = g_lockFreeQueueBenchmarkProfile == 0 ? static_cast<int64_t>(width + 1) : -1;
        if (queue.configurePool(config) != LockFreeQueueSuccess || queue.tryInit() != LockFreeQueueSuccess)
        {
            return false;
        }
    }
#endif
    Message input;
    input.m_sequence = 0;
    input.m_body = body;
    Message output;
    for (size_t i = 0; i < width; ++i)
    {
        LockFreeQueueBenchmarkPush::checked(queue, input);
    }
    queue.clear();
    std::vector<uint64_t> pushes, pops;
    pushes.reserve((count + 63) / 64);
    pops.reserve((count + 63) / 64);
#if LOCK_FREE_QUEUE_BENCH_VARIANT >= 3
    LockFreeQueueProducer<Message> producer;
    if (queue.tryAcquireProducer(producer) != LockFreeQueueSuccess)
    {
        return false;
    }
#endif
    bool valid = true;
    uint64_t cpuStart = 0;
    const uint64_t began = begin(cpuStart);
    for (size_t round = 0; round < count / width; ++round)
    {
        for (size_t i = 0; i < width; ++i)
        {
            input.m_sequence = round * width + i;
            const uint64_t started = i % 64 == 0 ? LockFreeQueueMeasurements::ticks() : 0;
#if LOCK_FREE_QUEUE_BENCH_VARIANT <= 1
            LockFreeQueueBenchmarkPush::checked(queue, input);
#elif LOCK_FREE_QUEUE_BENCH_VARIANT == 2
            valid = queue.tryPush(input) == LockFreeQueueSuccess && valid;
#elif LOCK_FREE_QUEUE_BENCH_VARIANT == 3
            valid = producer.tryPush(input) == LockFreeQueueSuccess && valid;
#else
            LockFreeQueueBenchmarkPush::checked(producer, input);
#endif
            if (i % 64 == 0)
            {
                pushes.push_back(LockFreeQueueMeasurements::ticks() - started);
            }
        }
        for (size_t i = 0; i < width; ++i)
        {
            const uint64_t started = i % 64 == 0 ? LockFreeQueueMeasurements::ticks() : 0;
            const bool popped = queue.pop(&output);
            if (i % 64 == 0)
            {
                pops.push_back(LockFreeQueueMeasurements::ticks() - started);
            }
            valid = popped && output.m_sequence == round * width + i && output.m_body == body && valid;
        }
    }
    valid = queue.empty() && valid;
    finish(name, began, cpuStart, count, valid);
    LockFreeQueueMeasurements::latency("payload_push", pushes);
    LockFreeQueueMeasurements::latency("payload_pop", pops);
#if LOCK_FREE_QUEUE_BENCH_VARIANT >= 3
    producer.release();
#endif
    return valid;
}

inline bool LockFreeQueuePerformanceExtra::run()
{
    LockFreeQueueMeasurements::resources("before_extra");
    bool valid = LockFreeQueueMeasurements::burst(1, 2000000);
    valid = LockFreeQueueMeasurements::burst(4096, 256) && valid;
    valid = LockFreeQueueMeasurements::burst(65536, 16) && valid;
    valid = observe(1) && valid;
    valid = observe(16) && valid;
    valid = observe(128) && valid;
    valid = lifecycle() && valid;
    valid = rejection() && valid;
    valid = registration() && valid;
    std::array<uint64_t, 32> inlineBody;
    inlineBody.fill(9);
    valid = payload("payload_inline256", inlineBody, 262144) && valid;
    valid = payload("payload_string64", std::string(64, 'x'), 262144) && valid;
    valid = payload("payload_string1024", std::string(1024, 'x'), 131072) && valid;
    valid = payload("payload_string16384", std::string(16384, 'x'), 8192) && valid;
    valid = payload("payload_shared256kb", std::make_shared<std::vector<int32_t>>(65536, 9), 262144) && valid;
    const size_t counts[] = { 1, 1024, 65536, 262144 };
    for (size_t i = 0; i < 4; ++i)
    {
        valid = LockFreeQueueMeasurements::backlog(counts[i]) && valid;
    }
    // These legacy supplementary paths explicitly test the general dynamic pool.
    valid = LockFreeQueueMeasurements::poolPaths() && valid;
    valid = LockFreeQueueMeasurements::payloadCost() && valid;
    LockFreeQueueMeasurements::resources("after_extra");
    std::cout << (valid ? "PASS" : "FAIL") << " suite=extra" << std::endl;
    return valid;
}
