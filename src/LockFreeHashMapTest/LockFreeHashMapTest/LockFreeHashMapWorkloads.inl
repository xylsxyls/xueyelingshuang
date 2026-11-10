#pragma once

template <typename H>
bool LockFreeHashMapWorkloads::run(const char* name, size_t buckets, int threads, int iterations, int writeEvery, int hotHistory)
{
    LockFreeHashMap<int, int, H> map(buckets);
    for (int key = 0; key < threads * 64; ++key)
    {
        if (!map.set(key, key))
        {
            return false;
        }
    }
    for (int i = 0; i < hotHistory; ++i)
    {
        if (!map.set(0, 0))
        {
            return false;
        }
    }
    std::vector<std::vector<double>> samples(threads);
    std::vector<int> errors(threads, 0);
    std::vector<LockFreeHashMapTestThread> workers;
    workers.reserve(threads);
    for (int t = 0; t < threads; ++t)
    {
        samples[t].reserve((iterations + 63) / 64);
    }
    std::atomic<bool> start(false);
    std::atomic<int> ready(0);
    try
    {
        for (int t = 0; t < threads; ++t)
        {
            // 无参数工作函数借用本用例夹具，结果在连接线程后读取，无返回值
            workers.push_back(LockFreeHashMapTestThread([&, t]()
            {
                int expected[64];
                for (int i = 0; i < 64; ++i)
                {
                    expected[i] = t * 64 + i;
                }
                uint32_t random = UINT32_C(0x5eed1234) + t;
                ready.fetch_add(1);
                while (!start.load())
                {
                    std::this_thread::yield();
                }
                for (int i = 0; i < iterations; ++i)
                {
                    random = random * 1664525U + 1013904223U;
                    int index = hotHistory != 0 ? 1 + int((random >> 12) % 63) : int((random >> 12) % 64);
                    int key = t * 64 + index;
                    // 采样周期与写入周期 2 和 20 互质，避免只采到写操作
                    bool timed = i % 67 == 0;
                    double begin = timed ? timestamp() : 0;
                    if (writeEvery != 0 && i % writeEvery == 0)
                    {
                        int value = t * iterations + i + 1024;
                        if (map.set(key, value))
                        {
                            expected[index] = value;
                        }
                        else
                        {
                            ++errors[t];
                        }
                    }
                    else
                    {
                        int value = -1;
                        if (!map.get(key, value) || value != expected[index])
                        {
                            ++errors[t];
                        }
                    }
                    if (timed)
                    {
                        samples[t].push_back((timestamp() - begin) * 1000000);
                    }
                }
            }));
        }
    }
    catch (...)
    {
        start.store(true);
        for (size_t i = 0; i < workers.size(); ++i)
        {
            workers[i].join();
        }
        throw;
    }
    while (ready.load() != threads)
    {
        std::this_thread::yield();
    }
    double begin = timestamp();
    start.store(true);
    for (size_t i = 0; i < workers.size(); ++i)
    {
        workers[i].join();
    }
    double elapsed = timestamp() - begin;
    std::vector<double> combined;
    int failed = 0;
    for (int t = 0; t < threads; ++t)
    {
        combined.insert(combined.end(), samples[t].begin(), samples[t].end());
        failed += errors[t];
    }
    std::sort(combined.begin(), combined.end());
    size_t retained = map.retainedNodeCount();
    map.clear();
    std::cout << "WORKLOAD name=" << name << " threads=" << threads << " buckets=" << buckets
        << " iterations=" << iterations << " write_every=" << writeEvery << " hot_history=" << hotHistory
        << " ops_per_second=" << double(threads) * iterations / elapsed
        << " p50_us=" << combined[combined.size() / 2] << " p95_us=" << combined[combined.size() * 95 / 100]
        << " p99_us=" << combined[combined.size() * 99 / 100] << " retained_units=" << retained
        << " after_clear_units=" << map.retainedNodeCount() << " errors=" << failed << std::endl;
    return failed == 0 && LockFreeHashMapTestThread::failures() == 0;
}

template <typename H>
bool LockFreeHashMapWorkloads::runOwned(const char* name, size_t buckets, int threads, int iterations, int writeEvery, int hotHistory)
{
    std::vector<std::shared_ptr<const int>> candidates;
    for (int i = 0; i < threads * 64; ++i)
    {
        candidates.push_back(std::make_shared<int>(i));
    }
    LockFreeHashMap<int, std::shared_ptr<const int>, H> map(buckets);
    for (int key = 0; key < threads * 64; ++key)
    {
        if (!map.set(key, candidates[key]))
        {
            return false;
        }
    }
    for (int i = 0; i < hotHistory; ++i)
    {
        if (!map.set(0, candidates[0]))
        {
            return false;
        }
    }
    std::vector<std::vector<double>> samples(threads);
    std::vector<int> errors(threads, 0);
    std::vector<LockFreeHashMapTestThread> workers;
    workers.reserve(threads);
    for (int t = 0; t < threads; ++t)
    {
        samples[t].reserve((iterations + 63) / 64);
    }
    std::atomic<bool> start(false);
    std::atomic<int> ready(0);
    try
    {
        for (int t = 0; t < threads; ++t)
        {
            // 无参数工作函数借用本用例夹具，结果在连接线程后读取，无返回值
            workers.push_back(LockFreeHashMapTestThread([&, t]()
            {
                std::shared_ptr<const int> expected[64];
                for (int i = 0; i < 64; ++i)
                {
                    expected[i] = candidates[t * 64 + i];
                }
                uint32_t random = UINT32_C(0x5eed1234) + t;
                ready.fetch_add(1);
                while (!start.load())
                {
                    std::this_thread::yield();
                }
                for (int i = 0; i < iterations; ++i)
                {
                    random = random * 1664525U + 1013904223U;
                    int index = hotHistory != 0 ? 1 + int((random >> 12) % 63) : int((random >> 12) % 64);
                    int key = t * 64 + index;
                    // 采样周期与写入周期 2 和 20 互质，避免只采到写操作
                    bool timed = i % 67 == 0;
                    double begin = timed ? timestamp() : 0;
                    if (writeEvery != 0 && i % writeEvery == 0)
                    {
                        std::shared_ptr<const int> value = candidates[(i + t) % candidates.size()];
                        if (map.set(key, value))
                        {
                            expected[index] = value;
                        }
                        else
                        {
                            ++errors[t];
                        }
                    }
                    else
                    {
                        std::shared_ptr<const int> value;
                        if (!map.get(key, value) || value != expected[index])
                        {
                            ++errors[t];
                        }
                    }
                    if (timed)
                    {
                        samples[t].push_back((timestamp() - begin) * 1000000);
                    }
                }
            }));
        }
    }
    catch (...)
    {
        start.store(true);
        for (size_t i = 0; i < workers.size(); ++i)
        {
            workers[i].join();
        }
        throw;
    }
    while (ready.load() != threads)
    {
        std::this_thread::yield();
    }
    double begin = timestamp();
    start.store(true);
    for (size_t i = 0; i < workers.size(); ++i)
    {
        workers[i].join();
    }
    double elapsed = timestamp() - begin;
    std::vector<double> combined;
    int failed = 0;
    for (int t = 0; t < threads; ++t)
    {
        combined.insert(combined.end(), samples[t].begin(), samples[t].end());
        failed += errors[t];
    }
    std::sort(combined.begin(), combined.end());
    size_t retained = map.retainedNodeCount();
    map.clear();
    std::cout << "WORKLOAD name=" << name << " threads=" << threads << " buckets=" << buckets
        << " iterations=" << iterations << " write_every=" << writeEvery << " hot_history=" << hotHistory
        << " ops_per_second=" << double(threads) * iterations / elapsed
        << " p50_us=" << combined[combined.size() / 2] << " p95_us=" << combined[combined.size() * 95 / 100]
        << " p99_us=" << combined[combined.size() * 99 / 100] << " retained_units=" << retained
        << " after_clear_units=" << map.retainedNodeCount() << " errors=" << failed << std::endl;
    return failed == 0 && LockFreeHashMapTestThread::failures() == 0;
}