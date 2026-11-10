#pragma once

template <typename T>
T LockFreeHashMapPerfData<T>::make(size_t marker)
{
    return T(marker);
}

template <typename K>
size_t LockFreeHashMapPerfCollisionHash<K>::operator()(const K& key) const
{
    (void)key;
    return 0;
}

template <typename K, typename V>
bool LockFreeHashMapPerformance::select(const LockFreeHashMapPerfSpec& spec, int32_t repeats)
{
#if defined(LOCKFREEHASHMAP_PERF_MEMORY)
    (void)repeats;
    return memory<K, V>(spec);
#else
    return spec.m_collision ? measure<K, V, LockFreeHashMapPerfCollisionHash<K>>(spec, repeats) :
        measure<K, V, std::hash<K>>(spec, repeats);
#endif
}

template <typename K, typename V, typename H>
bool LockFreeHashMapPerformance::measure(const LockFreeHashMapPerfSpec& spec, int32_t repeats)
{
    std::vector<K> keys;
    std::vector<V> candidates;
    keys.reserve(spec.m_keys * 2);
    candidates.reserve(spec.m_keys * 2);
    for (size_t i = 0; i < spec.m_keys * 2; ++i)
    {
        keys.push_back(LockFreeHashMapPerfData<K>::make(i));
        candidates.push_back(LockFreeHashMapPerfData<V>::make(i + 1));
    }
    std::vector<double> samples;
    double ignored = 0;
    uint64_t ignoredCalls = 0;
    if (!batch<K, V, H>(spec, keys, candidates, false, samples, ignored, ignoredCalls))
    {
        return false;
    }
    double elapsed = 0;
    double minimum = 1e100;
    double maximum = 0;
    uint64_t extraCalls = 0;
    for (int32_t repeat = 0; repeat < repeats; ++repeat)
    {
        double seconds = 0;
        uint64_t retries = 0;
        if (!batch<K, V, H>(spec, keys, candidates, false, samples, seconds, retries) || seconds <= 0)
        {
            return false;
        }
        elapsed += seconds;
        extraCalls += retries;
        minimum = (std::min)(minimum, seconds);
        maximum = (std::max)(maximum, seconds);
    }
    int32_t latencyBatches = 1;
    for (int32_t repeat = 0; repeat < latencyBatches; ++repeat)
    {
        if (!batch<K, V, H>(spec, keys, candidates, true, samples, ignored, ignoredCalls))
        {
            return false;
        }
    }
    uint64_t calls = uint64_t(spec.m_iterations) * repeats * LockFreeHashMapPerfModels::callsPerIteration(spec);
    std::cout << std::setprecision(12) << "PERF id=" << spec.m_id << " name=" << spec.m_name << " repeats=" << repeats
        << " calls=" << calls << " seconds=" << elapsed << " ops=" << double(calls) / elapsed
        << " p50_us=" << LockFreeHashMapPerfModels::percentile(samples, 50) * 1000000
        << " p95_us=" << LockFreeHashMapPerfModels::percentile(samples, 95) * 1000000
        << " p99_us=" << LockFreeHashMapPerfModels::percentile(samples, 99) * 1000000
        << " samples=" << samples.size() << " batch_min_s=" << minimum << " batch_max_s=" << maximum
        << " extra_calls=" << extraCalls << " attempted_calls=" << calls + extraCalls
        << " iteration_multiplier=" << spec.m_iterationMultiplier << " errors=0" << std::endl;
    return true;
}

template <typename K, typename V, typename H, bool ReadOnly>
bool LockFreeHashMapPerformance::batch(const LockFreeHashMapPerfSpec& spec, const std::vector<K>& keys,
    const std::vector<V>& candidates, bool sampled, std::vector<double>& samples, double& seconds, uint64_t& extraCalls)
{
    // 纯读批次在计时前选择独立实例，循环不携带其他操作的分派和写入分支。
    if (!ReadOnly && spec.m_operation == LockFreeHashMapPerfMixed && spec.m_readPercent == 100)
    {
        return batch<K, V, H, true>(spec, keys, candidates, sampled, samples, seconds, extraCalls);
    }
    if (spec.m_threads == 1)
    {
        LockFreeHashMapPerfModels::bindWorker(0);
    }
    if (spec.m_operation == LockFreeHashMapPerfClear)
    {
        extraCalls = 0;
        return clearBatch<K, V, H>(spec, keys, candidates, sampled, samples, seconds);
    }
#if defined(LOCKFREEHASHMAP_PERF_STD_MUTEX)
    typedef LockFreeHashMapStdMutex<K, V, H> Map;
#else
    typedef LockFreeHashMap<K, V, H> Map;
#endif
    Map map(spec.m_buckets);
    if (spec.m_operation != LockFreeHashMapPerfInsert)
    {
        for (size_t i = 0; i < spec.m_keys; ++i)
        {
            if (!map.set(keys[i], candidates[i * 2]))
            {
                return false;
            }
        }
    }
    for (size_t i = 0; i < spec.m_history; ++i)
    {
        if (!map.set(keys[0], candidates[0]))
        {
            return false;
        }
    }
    size_t bucketsBefore = map.bucketCount();
    size_t owned = spec.m_sharedKeys ? spec.m_keys : spec.m_keys / spec.m_threads;
    size_t iterations = spec.m_iterations / spec.m_threads;
    std::vector<std::vector<unsigned char>> states(spec.m_threads, std::vector<unsigned char>(owned, 0));
    std::vector<std::vector<double>> measured(spec.m_threads);
    std::vector<uint64_t> failures(spec.m_threads, 0);
    std::vector<uint64_t> retries(spec.m_threads, 0);
    std::vector<double> finished(spec.m_threads, 0);
    std::vector<std::vector<uint64_t>> firstFailure(spec.m_threads, std::vector<uint64_t>(5, 0));
    std::vector<LockFreeHashMapTestThread> workers;
    workers.reserve(spec.m_threads);
    for (int32_t thread = 0; thread < spec.m_threads; ++thread)
    {
        measured[thread].reserve(iterations < 2048 ? iterations : iterations / 127 + 1);
    }
    std::atomic<int32_t> ready(0);
    std::atomic<bool> start(false);
    std::atomic<bool> cancelled(false);
    int previousFailures = LockFreeHashMapTestThread::failures();
    // 工作函数借用本批夹具；单线程直接运行，多线程共用相同循环，不计线程退出时间。
    // 参数 thread 为工作线程编号，output 借用该线程的局部值；函数无返回值，异常由所有者或工作线程捕获。
    auto work = [&](int32_t thread, V& output)
    {
        uint32_t random = UINT32_C(0x5eed1234) + uint32_t(thread);
        size_t base = spec.m_sharedKeys ? 0 : size_t(thread) * owned;
        uint64_t errors = 0;
        uint64_t extra = 0;
        ready.fetch_add(1);
        while (!start.load())
        {
            std::this_thread::yield();
        }
        if (cancelled.load())
        {
            return;
        }
        for (size_t iteration = 0; iteration < iterations; ++iteration)
        {
            random = random * 1664525U + 1013904223U;
            size_t local = size_t(random >> 8) % owned;
            if (spec.m_history != 0)
            {
                local = 1 + size_t(random >> 8) % (owned - 1);
            }
            if (spec.m_operation == LockFreeHashMapPerfInsert || spec.m_operation == LockFreeHashMapPerfErase)
            {
                local = iteration;
            }
            size_t index = base + local;
            bool timed = sampled && (iterations < 2048 || iteration % 127 == 0);
            double begin = timed ? LockFreeHashMapPerfModels::timestamp() : 0;
            bool ok = true;
            uint64_t reason = 100 + spec.m_operation;
            switch (ReadOnly ? LockFreeHashMapPerfMixed : spec.m_operation)
            {
            case LockFreeHashMapPerfMixed:
            {
                if (ReadOnly || iteration % 100 < size_t(spec.m_readPercent))
                {
                    bool found = map.get(keys[index], output);
                    ok = found && (spec.m_sharedKeys ?
                        (output == candidates[index * 2] || output == candidates[index * 2 + 1]) :
                        output == candidates[index * 2 + states[thread][local]]);
                    reason = !found ? 1 : 2;
                }
                else
                {
                    unsigned char next = static_cast<unsigned char>(states[thread][local] ^ 1);
#if defined(LOCKFREEHASHMAP_PERF_DIAGNOSTIC) && defined(LOCKFREEHASHMAP_PERF_CURRENT)
                    // 诊断编译才展开失败状态，正式比较仍使用两版共有的 bool 接口
                    LockFreeHashMapResult written = map.trySet(keys[index], candidates[index * 2 + next]);
                    ok = written.m_committed;
                    reason = 1000 + written.m_status;
#else
                    ok = write(map, keys[index], candidates[index * 2 + next], extra);
                    reason = 3;
#endif
                    if (ok)
                    {
                        states[thread][local] = next;
                    }
                }
                break;
            }
            case LockFreeHashMapPerfMiss:
            {
                ok = !map.get(keys[spec.m_keys + index], output);
                break;
            }
            case LockFreeHashMapPerfContains:
            {
                ok = map.contains(keys[index]);
                break;
            }
            case LockFreeHashMapPerfContainsMiss:
            {
                ok = !map.contains(keys[spec.m_keys + index]);
                break;
            }
            case LockFreeHashMapPerfInsert:
            {
                ok = map.insert(keys[index], candidates[index * 2]);
                break;
            }
            case LockFreeHashMapPerfInsertExists:
            {
                ok = !map.insert(keys[index], candidates[index * 2]);
                break;
            }
            case LockFreeHashMapPerfErase:
            {
                ok = map.erase(keys[index]);
                break;
            }
            case LockFreeHashMapPerfChurn:
            {
                ok = map.erase(keys[index]) && write(map, keys[index], candidates[index * 2], extra) &&
                    map.get(keys[index], output) && output == candidates[index * 2];
                break;
            }
            case LockFreeHashMapPerfSize:
            {
                ok = map.size() == spec.m_keys;
                break;
            }
            case LockFreeHashMapPerfVisit:
            {
                size_t visited = 0;
                // 借用键值并计数，检查回调次数及接口返回值一致
                size_t count = map.forEach([&visited](const K& key, const V& value)
                {
                    (void)key;
                    (void)value;
                    ++visited;
                });
                ok = count == spec.m_keys && visited == count;
                break;
            }
            case LockFreeHashMapPerfKeys:
            {
                ok = map.keys().size() == spec.m_keys;
                break;
            }
            case LockFreeHashMapPerfValues:
            {
                ok = map.values().size() == spec.m_keys;
                break;
            }
            case LockFreeHashMapPerfPairs:
            {
                ok = map.toVector().size() == spec.m_keys;
                break;
            }
            case LockFreeHashMapPerfClear:
            {
                map.clear();
                break;
            }
            case LockFreeHashMapPerfRefill:
            {
                map.clear();
                for (size_t i = 0; i < spec.m_keys; ++i)
                {
                    ok = write(map, keys[i], candidates[i * 2], extra) && ok;
                }
                break;
            }
#if defined(LOCKFREEHASHMAP_PERF_CURRENT)
            case LockFreeHashMapPerfFind:
            case LockFreeHashMapPerfConditionalExists:
            case LockFreeHashMapPerfCompare:
            case LockFreeHashMapPerfConditionalErase:
            {
                typename Map::Snapshot snapshot;
                if (spec.m_operation == LockFreeHashMapPerfConditionalExists)
                {
                    ok = map.insertIfAbsent(keys[index], candidates[index * 2], snapshot).m_status == LockFreeHashMapExists;
                }
                else
                {
                    ok = map.find(keys[index], snapshot).m_status == LockFreeHashMapOk;
                }
                ok = ok && snapshot.m_value && *snapshot.m_value == candidates[index * 2];
                if (ok && spec.m_operation == LockFreeHashMapPerfCompare)
                {
                    ok = false;
                    for (size_t attempt = 0; attempt < 1024; ++attempt)
                    {
                        LockFreeHashMapResult changed = map.compareExchange(keys[index], snapshot.m_version, candidates[index * 2]);
                        if (changed.m_committed)
                        {
                            ok = true;
                            break;
                        }
                        ++extra;
                        if (changed.m_status != LockFreeHashMapReservationBusy && changed.m_status != LockFreeHashMapProtectionUnavailable)
                        {
                            break;
                        }
                    }
                }
                if (ok && spec.m_operation == LockFreeHashMapPerfConditionalErase)
                {
                    ok = map.eraseIf(keys[index], snapshot.m_version).m_committed;
                    if (ok)
                    {
                        ok = false;
                        for (size_t attempt = 0; attempt < 1024; ++attempt)
                        {
                            LockFreeHashMapResult inserted = map.insertIfAbsent(keys[index], candidates[index * 2], snapshot);
                            if (inserted.m_committed)
                            {
                                ok = true;
                                break;
                            }
                            ++extra;
                            if (inserted.m_status != LockFreeHashMapReservationBusy && inserted.m_status != LockFreeHashMapProtectionUnavailable)
                            {
                                break;
                            }
                        }
                    }
                }
                break;
            }
#endif
            default:
            {
                ok = false;
                break;
            }
            }
            if (!ok)
            {
                if (++errors == 1)
                {
                    firstFailure[thread][0] = reason;
                    firstFailure[thread][1] = iteration;
                    firstFailure[thread][2] = index;
                    firstFailure[thread][3] = states[thread][local];
                    firstFailure[thread][4] = output == candidates[index * 2] ? 0 : (output == candidates[index * 2 + 1] ? 1 : 2);
                }
            }
            if (timed)
            {
                measured[thread].push_back(LockFreeHashMapPerfModels::timestamp() - begin);
            }
        }
        failures[thread] = errors;
        retries[thread] = extra;
        finished[thread] = LockFreeHashMapPerfModels::timestamp();
    };
    double begin = 0;
    try
    {
        if (spec.m_threads == 1)
        {
            // 输出在计时前构造，避免把测试变量的分配成本计为容器操作。
            V output(candidates[0]);
            start.store(true);
            begin = LockFreeHashMapPerfModels::timestamp();
            work(0, output);
        }
        else
        {
            for (int32_t thread = 0; thread < spec.m_threads; ++thread)
            {
                // 捕获线程编号副本，夹具借用期由下方 join 覆盖。
                workers.push_back(LockFreeHashMapTestThread([&, thread]()
                {
                    bool entered = false;
                    try
                    {
                        LockFreeHashMapPerfModels::bindWorker(static_cast<size_t>(thread));
                        // 各线程使用自己的栈输出，避免相邻输出格的伪共享掩盖容器读取成本。
                        // 构造发生在ready之前，失败也必须完成入场计数，防止控制线程无限等待。
                        V output(candidates[0]);
                        entered = true;
                        work(thread, output);
                    }
                    catch (...)
                    {
                        cancelled.store(true);
                        if (!entered)
                        {
                            ready.fetch_add(1);
                        }
                        throw;
                    }
                }));
            }
        }
    }
    catch (...)
    {
        cancelled.store(true);
        start.store(true);
        for (size_t i = 0; i < workers.size(); ++i)
        {
            workers[i].join();
        }
        throw;
    }
    while (ready.load() != spec.m_threads)
    {
        std::this_thread::yield();
    }
    try
    {
        if (spec.m_threads != 1)
        {
            begin = LockFreeHashMapPerfModels::timestamp();
        }
    }
    catch (...)
    {
        cancelled.store(true);
        start.store(true);
        for (size_t i = 0; i < workers.size(); ++i)
        {
            workers[i].join();
        }
        throw;
    }
    start.store(true);
    for (size_t i = 0; i < workers.size(); ++i)
    {
        workers[i].join();
    }
    seconds = *std::max_element(finished.begin(), finished.end()) - begin;
    if (*std::min_element(finished.begin(), finished.end()) <= begin)
    {
        return false;
    }
    if (sampled)
    {
        // 三版均在计时结束后记录实际桶数；std 保留正常自动扩桶，不伪称始终只有请求的桶数
        std::cout << "BUCKETS requested=" << spec.m_buckets << " before=" << bucketsBefore
            << " after=" << map.bucketCount() << std::endl;
    }
    uint64_t errors = 0;
    extraCalls = 0;
    for (int32_t thread = 0; thread < spec.m_threads; ++thread)
    {
        errors += failures[thread];
        extraCalls += retries[thread];
        samples.insert(samples.end(), measured[thread].begin(), measured[thread].end());
    }
    bool empty = spec.m_operation == LockFreeHashMapPerfErase || spec.m_operation == LockFreeHashMapPerfClear;
    V output = candidates[0];
    for (size_t i = 0; i < spec.m_keys; ++i)
    {
        bool found = map.get(keys[i], output);
        if (empty ? found : (!found || !(output == candidates[i * 2] || output == candidates[i * 2 + 1])))
        {
            ++errors;
        }
    }
    if (errors != 0 || LockFreeHashMapTestThread::failures() != previousFailures)
    {
        std::cerr << "INVALID id=" << spec.m_id << " errors=" << errors << std::endl;
        for (int32_t thread = 0; thread < spec.m_threads; ++thread)
        {
            if (failures[thread])
            {
                std::cerr << "FIRST_FAILURE thread=" << thread << " reason=" << firstFailure[thread][0]
                    << " iteration=" << firstFailure[thread][1] << " key_index=" << firstFailure[thread][2]
                    << " expected_bit=" << firstFailure[thread][3] << " observed_bit=" << firstFailure[thread][4] << std::endl;
            }
        }
        return false;
    }
    return true;
}

template <typename K, typename V, typename H>
bool LockFreeHashMapPerformance::clearBatch(const LockFreeHashMapPerfSpec& spec, const std::vector<K>& keys,
    const std::vector<V>& candidates, bool sampled, std::vector<double>& samples, double& seconds)
{
#if defined(LOCKFREEHASHMAP_PERF_STD_MUTEX)
    typedef LockFreeHashMapStdMutex<K, V, H> Map;
#else
    typedef LockFreeHashMap<K, V, H> Map;
#endif
    if (spec.m_threads != 1 || spec.m_iterations == 0 || spec.m_iterations > 64)
    {
        return false;
    }
    std::vector<std::unique_ptr<Map>> maps;
    maps.reserve(spec.m_iterations);
    for (size_t i = 0; i < spec.m_iterations; ++i)
    {
        std::unique_ptr<Map> map(new Map(spec.m_buckets));
        for (size_t key = 0; key < spec.m_keys; ++key)
        {
            if (!map->set(keys[key], candidates[key * 2]))
            {
                return false;
            }
        }
        maps.push_back(std::move(map));
    }
    // 建表和最后析构均在计时外；每次 clear 面对完整活跃表，不反复测已空表。
    double begin = LockFreeHashMapPerfModels::timestamp();
    for (size_t i = 0; i < maps.size(); ++i)
    {
        double before = sampled ? LockFreeHashMapPerfModels::timestamp() : 0;
        maps[i]->clear();
        if (sampled)
        {
            samples.push_back(LockFreeHashMapPerfModels::timestamp() - before);
        }
    }
    seconds = LockFreeHashMapPerfModels::timestamp() - begin;
    V output = candidates[0];
    for (size_t i = 0; i < maps.size(); ++i)
    {
        for (size_t key = 0; key < spec.m_keys; ++key)
        {
            if (maps[i]->get(keys[key], output))
            {
                return false;
            }
        }
    }
    return seconds > 0;
}

template <typename M, typename K, typename V>
bool LockFreeHashMapPerformance::write(M& map, const K& key, const V& value, uint64_t& extraCalls)
{
    for (size_t attempt = 0; attempt < 1024; ++attempt)
    {
        if (map.set(key, value))
        {
            return true;
        }
        ++extraCalls;
    }
    return false;
}

#if defined(LOCKFREEHASHMAP_PERF_MEMORY)
template <typename K, typename V>
bool LockFreeHashMapPerformance::memory(const LockFreeHashMapPerfSpec& spec)
{
    size_t baseline = 0;
#if defined(LOCKFREEHASHMAP_PERF_STD_MUTEX)
    // v120 首次加锁可能初始化进程级运行库；先用纯 mutex 对照，不创建任何 Map
    LockFreeHashMapPerfMemory::s_tracking = true;
    for (size_t round = 0; round < 3; ++round)
    {
        {
            std::mutex control;
            std::lock_guard<std::mutex> guard(control);
        }
        size_t observed = LockFreeHashMapPerfMemory::s_bytes;
        if (round != 0 && observed != baseline)
        {
            LockFreeHashMapPerfMemory::s_tracking = false;
            std::cerr << "MEMORY_RUNTIME unstable baseline=" << baseline << " observed=" << observed << std::endl;
            return false;
        }
        baseline = observed;
    }
    LockFreeHashMapPerfMemory::s_tracking = false;
#endif
    // 保留累计字节计数和申请标记，不清零或忽略后来发生的真实释放
    std::cout << "MEMORY_RUNTIME retained=" << baseline << " control_net=0" << std::endl;
    std::vector<K> keys;
    std::vector<V> values;
    for (size_t i = 0; i < spec.m_keys; ++i)
    {
        keys.push_back(LockFreeHashMapPerfData<K>::make(i));
        values.push_back(LockFreeHashMapPerfData<V>::make(i + 1));
    }
    V output = values.back();
    size_t retained = 0, afterClear = 0, afterFree = 0;
    uint64_t beforeWork = 0, workAllocations = 0;
    uint64_t beforeMap = LockFreeHashMapPerfMemory::s_allocations;
    bool correct = true;
    LockFreeHashMapPerfMemory::s_peak = baseline;
    LockFreeHashMapPerfMemory::s_tracking = true;
    {
#if defined(LOCKFREEHASHMAP_PERF_STD_MUTEX)
        LockFreeHashMapStdMutex<K, V> map(spec.m_buckets);
#else
        LockFreeHashMap<K, V> map(spec.m_buckets);
#endif
        for (size_t i = 0; i < spec.m_keys; ++i)
        {
            correct = map.set(keys[i], values[i]) && correct;
        }
        beforeWork = LockFreeHashMapPerfMemory::s_allocations;
        for (size_t i = 0; i < spec.m_iterations; ++i)
        {
            size_t index = i % spec.m_keys;
            if (spec.m_operation == LockFreeHashMapPerfChurn)
            {
                correct = map.erase(keys[index]) && correct;
            }
            correct = map.set(keys[index], values[index]) && map.get(keys[index], output) && output == values[index] && correct;
        }
        workAllocations = LockFreeHashMapPerfMemory::s_allocations - beforeWork;
        correct = LockFreeHashMapPerfMemory::s_bytes >= baseline && correct;
        retained = LockFreeHashMapPerfMemory::s_bytes >= baseline ? LockFreeHashMapPerfMemory::s_bytes - baseline : 0;
        map.clear();
        correct = LockFreeHashMapPerfMemory::s_bytes >= baseline && correct;
        afterClear = LockFreeHashMapPerfMemory::s_bytes >= baseline ? LockFreeHashMapPerfMemory::s_bytes - baseline : 0;
        map.clearAndFreeMemory();
        correct = LockFreeHashMapPerfMemory::s_bytes >= baseline && correct;
        afterFree = LockFreeHashMapPerfMemory::s_bytes >= baseline ? LockFreeHashMapPerfMemory::s_bytes - baseline : 0;
    }
    LockFreeHashMapPerfMemory::s_tracking = false;
    correct = correct && LockFreeHashMapPerfMemory::s_bytes == baseline;
    size_t difference = LockFreeHashMapPerfMemory::s_bytes >= baseline ? LockFreeHashMapPerfMemory::s_bytes - baseline : baseline - LockFreeHashMapPerfMemory::s_bytes;
    std::cout << "MEM id=" << spec.m_id << " name=" << spec.m_name << " retained=" << retained
        << " peak=" << LockFreeHashMapPerfMemory::s_peak - baseline << " allocations=" << LockFreeHashMapPerfMemory::s_allocations - beforeMap
        << " work_allocations=" << workAllocations << " after_clear=" << afterClear << " after_free=" << afterFree
        << " net=" << difference << " errors=" << (correct ? 0 : 1) << std::endl;
    return correct;
}
#endif