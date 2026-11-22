#include "NetObjectPoolTest.h"
#include "NetInterface/NetObjectPool.h"
#include <thread>
#include <vector>
#include <stdexcept>
#include <cstdio>

NetObjectPoolTestValue::NetObjectPoolTestValue(std::atomic<uint32_t>& live, uint64_t value) :
m_live(&live),
m_value(value)
{
    m_live->fetch_add(1);
}

NetObjectPoolTestValue::~NetObjectPoolTestValue()
{
    m_live->fetch_sub(1);
}

void NetObjectPoolTest::run()
{
    concurrentGrowth();
    std::atomic<uint32_t> live(0);
    LockFreeHashMapPoolConfig config;
    config.initialCapacity = 8;
    config.maximumCapacity = 32;
    config.directoryCapacity = 8;
    config.protectionRecordCount = 16;
    config.lookupCacheCapacity = 0;
    NetObjectPool<NetObjectPoolTestValue> pool(config);
    if (pool.status() != NI_OK)
    {
        throw std::runtime_error("Object pool initialization");
    }
    std::vector<NetObjectReference<NetObjectPoolTestValue>> references(32);
    for (uint32_t index = 0; index < references.size(); ++index)
    {
        if (pool.create(references[index], live, index) != NI_OK || references[index]->m_value != index ||
            pool.create(references[index], live, index) != NI_INVALID_ARGUMENT)
        {
            throw std::runtime_error("Object pool allocation or nonempty output");
        }
    }
    NetObjectReference<NetObjectPoolTestValue> extra;
    if (pool.create(extra, live, 33) != NI_LIMIT_EXCEEDED || extra || live != 32 ||
        pool.statistics().capacity != 32 || pool.statistics().successfulGrowCount == 0)
    {
        throw std::runtime_error("Object pool growth or capacity");
    }
    NetObjectReference<NetObjectPoolTestValue> copy(references[0]);
    references[0].reset();
    if (live != 32 || pool.create(extra, live, 33) != NI_LIMIT_EXCEEDED)
    {
        throw std::runtime_error("Object pool returned a still borrowed slot");
    }
    NetObjectReference<NetObjectPoolTestValue> moved(std::move(copy));
    if (copy || !moved || moved->m_value != 0)
    {
        throw std::runtime_error("Object pool move");
    }
    void* detached = moved.storage();
    NetObjectPoolTestValue* borrowed = moved.release();
    if (moved || borrowed->m_value != 0 || pool.create(extra, live, 33) != NI_LIMIT_EXCEEDED)
    {
        throw std::runtime_error("Object pool detached ownership");
    }
    pool.release(detached);
    if (live != 31 || pool.create(extra, live, 33) != NI_OK)
    {
        throw std::runtime_error("Object pool exact final release or reuse");
    }
    references.clear();
    extra.reset();
    std::atomic<uint32_t> errors(0);
    std::vector<std::thread> threads;
    for (uint32_t producer = 0; producer < 4; ++producer)
    {
        // 各线程持有不同引用；反复构造、复制、移动、释放同一池中的对象
        threads.push_back(std::thread([&pool, &live, &errors, producer]()
        {
            for (uint32_t index = 0; index < 20000; ++index)
            {
                NetObjectReference<NetObjectPoolTestValue> value;
                if (pool.create(value, live, static_cast<uint64_t>(producer) * 20000 + index) != NI_OK)
                {
                    errors.fetch_add(1);
                    continue;
                }
                NetObjectReference<NetObjectPoolTestValue> a(value);
                NetObjectReference<NetObjectPoolTestValue> b;
                b = a;
                a = std::move(value);
                if (value || b.get() != a.get())
                {
                    errors.fetch_add(1);
                }
            }
        }));
    }
    for (size_t index = 0; index < threads.size(); ++index)
    {
        threads[index].join();
    }
    if (errors != 0 || live != 0 || pool.statistics().occupied != 0 ||
        pool.statistics().capacity != 32 || pool.statistics().activeProtectionRecords > 1)
    {
        throw std::runtime_error("Object pool concurrent ownership or resource leak");
    }
    config.maximumCapacity = -2;
    NetObjectPool<NetObjectPoolTestValue> invalid(config);
    if (invalid.status() != NI_INVALID_ARGUMENT || invalid.create(extra, live, 0) != NI_INVALID_ARGUMENT || extra)
    {
        throw std::runtime_error("Object pool invalid configuration");
    }
}

void NetObjectPoolTest::concurrentGrowth()
{
    for (uint32_t round = 0; round < 20; ++round)
    {
        LockFreeHashMapPoolConfig config;
        config.initialCapacity = 0;
        config.maximumCapacity = 8192;
        config.directoryCapacity = 128;
        config.lookupCacheCapacity = 0;
        NetObjectPool<NetObjectPoolTestValue> pool(config);
        std::atomic<uint32_t> live(0), ready(0), errors(0);
        std::atomic<bool> start(false);
        std::vector<NetObjectReference<NetObjectPoolTestValue>> references(4096);
        std::vector<std::thread> threads;
        for (uint32_t producer = 0; producer < 16; ++producer)
        {
            // 各生产者拥有独立输出槽；全员就绪后同时首次申请，消费在全部提交后开始
            threads.push_back(std::thread([&pool, &references, &live, &ready, &errors, &start, producer]()
            {
                ready.fetch_add(1);
                while (!start.load())
                {
                    std::this_thread::yield();
                }
                for (uint32_t offset = 0; offset < 256; ++offset)
                {
                    const size_t index = producer * 256 + offset;
                    if (pool.create(references[index], live, index) != NI_OK)
                    {
                        errors.fetch_add(1);
                    }
                }
            }));
        }
        while (ready.load() != 16)
        {
            std::this_thread::yield();
        }
        start.store(true);
        for (size_t index = 0; index < threads.size(); ++index)
        {
            threads[index].join();
        }
        if (errors != 0 || live != 4096)
        {
            throw std::runtime_error("Object pool concurrent cold growth falsely exhausted capacity");
        }
        for (size_t index = 0; index < references.size(); ++index)
        {
            if (references[index]->m_value != index)
            {
                throw std::runtime_error("Object pool concurrent cold growth ownership");
            }
        }
        references.clear();
        if (live != 0 || pool.statistics().occupied != 0)
        {
            throw std::runtime_error("Object pool concurrent cold growth reclamation");
        }
    }
}

#ifdef NET_OBJECT_POOL_TEST_STANDALONE
int main()
{
    try
    {
        NetObjectPoolTest::run();
        std::puts("Object pool tests passed");
        return 0;
    }
    catch (const std::exception& error)
    {
        std::printf("Object pool tests failed: %s\n", error.what());
        return 1;
    }
}
#endif