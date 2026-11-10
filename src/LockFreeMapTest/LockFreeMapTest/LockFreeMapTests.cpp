#include "LockFreeMapTests.h"
#include "LockFreeMapTestCopyKey.h"
#include <cstdlib>
#include <cstdio>
#include <stdexcept>
#include <vector>
#include <string>
#ifdef _WIN32
#include <TlHelp32.h>
#endif

// 仅单线程故障扫描阶段开启分配失败注入
std::atomic<int64_t> LockFreeMapTestFaults::m_failAfter(-1);
std::atomic<int> LockFreeMapTestFaults::m_enabled(0);
std::atomic<int> LockFreeMapTestFaults::m_pauseAllocation(-1);
std::atomic<uint64_t> LockFreeMapTestFaults::m_allocations(0);
std::atomic<int64_t> LockFreeMapTestFaults::m_outstanding(0);

void* operator new(size_t size)
{
    if (LockFreeMapTestFaults::m_enabled.load() != 0 && LockFreeMapTestFaults::m_pauseAllocation.load() >= 0 &&
        LockFreeMapTestFaults::m_pauseAllocation.fetch_sub(1) == 0)
    {
        LockFreeMapTestValue::m_entered.store(1);
        if (!LockFreeMapTests::wait(LockFreeMapTestValue::m_release))
        {
            throw std::bad_alloc();
        }
    }
    const int64_t fail = LockFreeMapTestFaults::m_failAfter.load();
    if (LockFreeMapTestFaults::m_enabled.load() != 0 && fail >= 0 && LockFreeMapTestFaults::m_failAfter.fetch_sub(1) == 0)
    {
        throw std::bad_alloc();
    }
    void* memory = std::malloc(size == 0 ? 1 : size);
    if (memory == nullptr)
    {
        throw std::bad_alloc();
    }
    LockFreeMapTestFaults::m_allocations.fetch_add(1);
    LockFreeMapTestFaults::m_outstanding.fetch_add(1);
    return memory;
}

void operator delete(void* memory) throw()
{
    if (memory != nullptr)
    {
        LockFreeMapTestFaults::m_outstanding.fetch_sub(1);
        std::free(memory);
    }
}

void* operator new[](size_t size)
{
    return ::operator new(size);
}

void operator delete[](void* memory) throw()
{
    ::operator delete(memory);
}

/** 显式接管非抛出申请，避免动态运行库绕过本测试的故障注入和计数
@param [in] size 申请字节数
@return 申请失败时返回空指针
*/
void* operator new(size_t size, const std::nothrow_t&) throw()
{
    try
    {
        return ::operator new(size);
    }
    catch (...)
    {
        return nullptr;
    }
}

void* operator new[](size_t size, const std::nothrow_t& tag) throw()
{
    return ::operator new(size, tag);
}

void operator delete(void* memory, const std::nothrow_t&) throw()
{
    ::operator delete(memory);
}

void operator delete[](void* memory, const std::nothrow_t&) throw()
{
    ::operator delete(memory);
}

std::atomic<int> LockFreeMapTestValue::m_live(0);
std::atomic<int> LockFreeMapTestValue::m_throwCopy(0);
std::atomic<int> LockFreeMapTestValue::m_throwAssignment(0);
std::atomic<int> LockFreeMapTestValue::m_pauseCopyValue(-1);
std::atomic<int> LockFreeMapTestValue::m_pauseDestructorValue(-1);
std::atomic<int> LockFreeMapTestValue::m_entered(0);
std::atomic<int> LockFreeMapTestValue::m_release(0);
std::atomic<size_t> LockFreeMapTestValue::m_timeouts(0);
std::atomic<int> LockFreeMapTestLess::m_throw(0);
std::atomic<int> LockFreeMapTestLess::m_throwConstructor(0);

LockFreeMapTestLess::LockFreeMapTestLess()
{
    if (m_throwConstructor.load() != 0)
    {
        throw std::runtime_error("comparator construction");
    }
}

LockFreeMapTestValue::LockFreeMapTestValue(int value) :
m_value(value),
m_pauseAssignment(false)
{
    m_live.fetch_add(1);
}

LockFreeMapTestValue::LockFreeMapTestValue(const LockFreeMapTestValue& other) :
m_value(other.m_value),
m_pauseAssignment(false)
{
    if (m_throwCopy.load() != 0)
    {
        throw std::runtime_error("injected payload copy");
    }
    if (m_pauseCopyValue.load() == other.m_value)
    {
        pause();
    }
    m_live.fetch_add(1);
}

LockFreeMapTestValue::~LockFreeMapTestValue()
{
    if (m_pauseDestructorValue.load() == m_value)
    {
        m_entered.store(1);
        LockFreeMapTests::wait(m_release);
    }
    m_live.fetch_sub(1);
}

LockFreeMapTestValue& LockFreeMapTestValue::operator=(const LockFreeMapTestValue& other)
{
    if (m_throwAssignment.load() != 0)
    {
        throw std::runtime_error("injected payload assignment");
    }
    if (m_pauseAssignment)
    {
        pause();
    }
    m_value = other.m_value;
    return *this;
}

void LockFreeMapTestValue::pause()
{
    m_entered.store(1);
    if (!LockFreeMapTests::wait(m_release))
    {
        throw std::runtime_error("controlled pause timed out");
    }
}

bool LockFreeMapTestLess::operator()(const LockFreeMapTestValue& left, const LockFreeMapTestValue& right) const
{
    if (m_throw.load() != 0)
    {
        throw std::runtime_error("injected comparator");
    }
    return left.m_value / 10 < right.m_value / 10;
}

bool LockFreeMapTests::wait(std::atomic<int>& flag)
{
    const uint64_t deadline = LockFreeMapClock::milliseconds() + 10000;
    while (flag.load() == 0 && LockFreeMapClock::milliseconds() < deadline)
    {
        try
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        catch (...)
        {
            LockFreeMapTestValue::m_timeouts.fetch_add(1);
            return false;
        }
    }
    const bool released = flag.load() != 0;
    if (!released)
    {
        LockFreeMapTestValue::m_timeouts.fetch_add(1);
    }
    return released;
}

bool LockFreeMapTests::tokens()
{
    LockFreeMap<LockFreeMapTestValue, int, LockFreeMapTestLess> map;
    LockFreeMapTestValue key(11);
    if (!map.set(key, 1) || map.insert(LockFreeMapTestValue(19), 2))
    {
        return false;
    }
    int value = 0;
    LockFreeMapToken old;
    if (map.lockFreeMapGet(LockFreeMapTestValue(15), value, old) != LOCK_FREE_MAP_OK || value != 1)
    {
        return false;
    }
    for (int iteration = 0; iteration < 2000; ++iteration)
    {
        if (!map.erase(key) || !map.set(key, iteration) || map.lockFreeMapErase(key, old) != LOCK_FREE_MAP_CONFLICT)
        {
            return false;
        }
    }
    map.clearAndFreeMemory();
    if (!map.set(key, 1) || map.lockFreeMapUpdate(key, old, 2) != LOCK_FREE_MAP_CONFLICT)
    {
        return false;
    }
    LockFreeMap<int, int, std::greater<int>> reverse;
    reverse.set(1, 10);
    reverse.set(9, 90);
    int maximum = 0;
    LockFreeMapToken token;
    return reverse.lockFreeMapLast(maximum, value, token) == LOCK_FREE_MAP_OK && maximum == 1 && value == 10;
}

bool LockFreeMapTests::exceptions()
{
    LockFreeMapTestLess::m_throwConstructor.store(1);
    LockFreeMap<LockFreeMapTestValue, int, LockFreeMapTestLess> failedConstruction;
    LockFreeMapTestLess::m_throwConstructor.store(0);
    if (failedConstruction.lockFreeMapSet(LockFreeMapTestValue(1), 1) != LOCK_FREE_MAP_USER_EXCEPTION ||
        failedConstruction.lockFreeMapConfigure(LockFreeMapConfig()) != LOCK_FREE_MAP_OK)
    {
        return false;
    }
    // 键复制失败时整数值已经构造
    // 两个池都必须回收此次未发布申请，并允许后续插入成功
    LockFreeMapTestValue::m_throwCopy.store(1);
    const LockFreeMapStatus keyCopy = failedConstruction.lockFreeMapSet(LockFreeMapTestValue(30), 9);
    LockFreeMapTestValue::m_throwCopy.store(0);
    const LockFreeMapStats refunded = failedConstruction.lockFreeMapStats();
    if (keyCopy != LOCK_FREE_MAP_USER_EXCEPTION || !failedConstruction.empty() ||
        refunded.nodes.retained != 0 || refunded.values.retained != 0 ||
        refunded.nodes.reusable != refunded.nodes.capacity || refunded.values.reusable != refunded.values.capacity ||
        !failedConstruction.set(LockFreeMapTestValue(30), 9))
    {
        return false;
    }
    LockFreeMap<LockFreeMapTestValue, LockFreeMapTestValue, LockFreeMapTestLess> map;
    LockFreeMapTestValue key(10), value(7), output;
    if (!map.set(key, value))
    {
        return false;
    }
    LockFreeMapToken token;
    map.lockFreeMapGet(key, output, token);
    LockFreeMapTestValue::m_throwCopy.store(1);
    const LockFreeMapStatus copy = map.lockFreeMapSet(LockFreeMapTestValue(30), value);
    LockFreeMapTestValue::m_throwCopy.store(0);
    LockFreeMapTestLess::m_throw.store(1);
    const LockFreeMapStatus compare = map.lockFreeMapErase(key, token);
    LockFreeMapTestLess::m_throw.store(0);
    LockFreeMapTestValue::m_throwAssignment.store(1);
    const LockFreeMapStatus extract = map.lockFreeMapExtract(key, output);
    LockFreeMapTestValue::m_throwAssignment.store(0);
    const size_t completed = map.forEach([](const LockFreeMapTestValue&, const LockFreeMapTestValue&) -> void
    {
        throw std::runtime_error("injected traversal callback");
    });
    size_t released = 0;
    map.lockFreeMapMaintain(4096, released);
    return copy == LOCK_FREE_MAP_USER_EXCEPTION && compare == LOCK_FREE_MAP_USER_EXCEPTION &&
    extract == LOCK_FREE_MAP_USER_EXCEPTION && completed == 0 && map.size() == 1 &&
    map.get(key, output) && output.m_value == 7 && map.lockFreeMapStats().activeOperations == 0 &&
    map.retainedNodeCount() == 1 && map.retainedValueCount() == 1;
}

bool LockFreeMapTests::sameKey()
{
    LockFreeMap<int, int> map;
    std::atomic<int> start(0), published(0), failed(0);
    std::vector<LockFreeMapTestThread> threads;
    threads.reserve(16);
    try
    {
        for (int id = 0; id < 16; ++id)
        {
            threads.push_back(LockFreeMapTestThread([&map, &start, &published, &failed, id]() -> void
            {
                if (!wait(start))
                {
                    failed.fetch_add(1);
                    return;
                }
                int actual = -1;
                LockFreeMapToken token;
                // 默认容量足够容纳全部候选；普通接口不得把 CAS 竞争交给测试代码重试
                const LockFreeMapStatus result = map.lockFreeMapGetOrInsert(7, id, actual, token);
                if (result == LOCK_FREE_MAP_COMMITTED)
                {
                    published.fetch_add(1);
                }
                if ((result != LOCK_FREE_MAP_COMMITTED && result != LOCK_FREE_MAP_EXISTS) || map.value(7, -2) != actual)
                {
                    failed.fetch_add(1);
                }
            }));
        }
    }
    catch (...)
    {
        failed.fetch_add(1);
    }
    start.store(1);
    for (size_t index = 0; index < threads.size(); ++index)
    {
        threads[index].join();
    }
    return failed.load() == 0 && published.load() == 1 && map.size() == 1;
}

bool LockFreeMapTests::pausedReader()
{
    LockFreeMap<int, LockFreeMapTestValue> map;
    LockFreeMapConfig config;
    config.nodes.initialCapacity = 2;
    config.nodes.maximumCapacity = 4;
    config.values.initialCapacity = 4;
    config.values.maximumCapacity = 4;
    if (map.lockFreeMapConfigure(config) != LOCK_FREE_MAP_OK || !map.set(1, LockFreeMapTestValue(11)))
    {
        return false;
    }
    LockFreeMapTestValue output;
    output.m_pauseAssignment = true;
    LockFreeMapTestValue::m_entered.store(0);
    LockFreeMapTestValue::m_release.store(0);
    bool read = false;
    LockFreeMapTestThread reader([&map, &output, &read]() -> void
    {
        read = map.get(1, output);
    });
    bool ok = wait(LockFreeMapTestValue::m_entered);
    for (int iteration = 0; iteration < 10000 && ok; ++iteration)
    {
        ok = map.set(1, LockFreeMapTestValue(iteration));
        ok = ok && map.retainedValueCount() <= 4;
    }
    LockFreeMapTestValue::m_release.store(1);
    reader.join();
    size_t released = 0;
    map.lockFreeMapMaintain(4096, released);
    ok = ok && read && output.m_value == 11 && map.retainedValueCount() == 1;
    LockFreeMap<int, LockFreeMapTestValue> limited;
    config.values.initialCapacity = 2;
    config.values.maximumCapacity = 2;
    if (!ok || limited.lockFreeMapConfigure(config) != LOCK_FREE_MAP_OK || !limited.set(1, LockFreeMapTestValue(11)))
    {
        return false;
    }
    LockFreeMapTestValue::m_entered.store(0);
    LockFreeMapTestValue::m_release.store(0);
    LockFreeMapTestThread held([&]()
    {
        read = limited.get(1, output);
    });
    ok = wait(LockFreeMapTestValue::m_entered) && limited.set(1, LockFreeMapTestValue(12)) &&
    limited.lockFreeMapSet(1, LockFreeMapTestValue(13)) == LOCK_FREE_MAP_CAPACITY && limited.erase(1);
    LockFreeMapTestValue::m_release.store(1);
    held.join();
    return ok && read && output.m_value == 11 && limited.set(1, LockFreeMapTestValue(14));
}

bool LockFreeMapTests::maximumRace()
{
    LockFreeMap<int, LockFreeMapTestValue> map;
    map.set(1, LockFreeMapTestValue(1));
    int key = 0;
    LockFreeMapTestValue value;
    LockFreeMapToken selected;
    map.lockFreeMapLast(key, value, selected);
    value.m_pauseAssignment = true;
    LockFreeMapTestValue::m_entered.store(0);
    LockFreeMapTestValue::m_release.store(0);
    LockFreeMapStatus result = LOCK_FREE_MAP_INVALID;
    LockFreeMapTestThread extractor([&]() -> void
    {
        result = map.lockFreeMapExtractLast(selected, key, value);
    });
    bool ok = wait(LockFreeMapTestValue::m_entered) && map.set(2, LockFreeMapTestValue(2));
    LockFreeMapTestValue::m_release.store(1);
    extractor.join();
    value.m_pauseAssignment = false;
    ok = ok && result == LOCK_FREE_MAP_CONFLICT && map.size() == 2;
    ok = ok && map.lockFreeMapLast(key, value, selected) == LOCK_FREE_MAP_OK && key == 2;
    return ok && map.lockFreeMapExtractLast(selected, key, value) == LOCK_FREE_MAP_COMMITTED && key == 2 && map.size() == 1;
}

bool LockFreeMapTests::clearRace()
{
    // 空根快捷清空与非空根交换清空均允许随后提交，但都不能覆盖清空后的其他写入
    for (size_t initial = 0; initial < 2; ++initial)
    {
        LockFreeMap<int, LockFreeMapTestValue> map;
        if (initial != 0 && !map.set(1, LockFreeMapTestValue(1)))
        {
            return false;
        }
        LockFreeMapTestValue::m_entered.store(0);
        LockFreeMapTestValue::m_release.store(0);
        LockFreeMapTestValue::m_pauseCopyValue.store(99);
        LockFreeMapStatus result = LOCK_FREE_MAP_INVALID;
        LockFreeMapTestThread writer([&]() -> void
        {
            result = map.lockFreeMapSet(1, LockFreeMapTestValue(99));
        });
        bool ok = wait(LockFreeMapTestValue::m_entered);
        const LockFreeMapStats constructing = map.lockFreeMapStats();
        ok = ok && constructing.values.retained == initial &&
            constructing.values.reusable + initial + 1 == constructing.values.capacity;
        map.clear();
        ok = ok && map.set(2, LockFreeMapTestValue(2));
        LockFreeMapTestValue::m_release.store(1);
        writer.join();
        LockFreeMapTestValue::m_pauseCopyValue.store(-1);
        LockFreeMapTestValue actual;
        if (!ok || result != LOCK_FREE_MAP_COMMITTED || !map.get(1, actual) || actual.m_value != 99 ||
            map.size() != 2 || !map.get(2, actual) || actual.m_value != 2)
        {
            return false;
        }
    }
    return true;
}
bool LockFreeMapTests::fullPools()
{
    LockFreeMap<int, LockFreeMapTestValue> map;
    LockFreeMapConfig config;
    config.nodes.initialCapacity = 2;
    config.nodes.maximumCapacity = 2;
    config.values.initialCapacity = 2;
    config.values.maximumCapacity = 2;
    if (map.lockFreeMapConfigure(config) != LOCK_FREE_MAP_OK || !map.set(1, LockFreeMapTestValue(10)) ||
        !map.set(2, LockFreeMapTestValue(20)))
    {
        return false;
    }
    if (map.lockFreeMapSet(1, LockFreeMapTestValue(30)) != LOCK_FREE_MAP_CAPACITY)
    {
        return false;
    }
    bool proxyFailed = false;
    try
    {
        map[1] = LockFreeMapTestValue(30);
    }
    catch (const LockFreeMapError& error)
    {
        proxyFailed = error.m_status == LOCK_FREE_MAP_CAPACITY;
    }
    if (!proxyFailed)
    {
        return false;
    }
    LockFreeMapTestValue output;
    if (map.lockFreeMapExtract(1, output) != LOCK_FREE_MAP_COMMITTED || output.m_value != 10 ||
        !map.set(3, LockFreeMapTestValue(30)))
    {
        return false;
    }
    map.clear();
    if (!map.set(4, LockFreeMapTestValue(40)))
    {
        return false;
    }
    map.clearAndFreeMemory();
    config.nodes.maximumCapacity = 1;
    config.nodes.initialCapacity = 1;
    config.values.maximumCapacity = 3;
    config.values.initialCapacity = 3;
    if (map.lockFreeMapConfigure(config) != LOCK_FREE_MAP_OK || !map.set(1, LockFreeMapTestValue(1)) ||
        map.lockFreeMapSet(2, LockFreeMapTestValue(2)) != LOCK_FREE_MAP_CAPACITY)
    {
        return false;
    }
    size_t released = 0;
    map.lockFreeMapMaintain(4096, released);
    return map.lockFreeMapStats().values.retained == 1 && map.set(1, LockFreeMapTestValue(3));
}

bool LockFreeMapTests::shrinking()
{
    LockFreeMap<int, int> map;
    LockFreeMapConfig config;
    std::atomic<int> enabled(0), contextErrors(0);
    config.nodes.initialCapacity = config.values.initialCapacity = 2;
    config.nodes.maximumCapacity = config.values.maximumCapacity = 10;
    config.nodes.blockRecordLimit = config.values.blockRecordLimit = 16;
    config.nodes.grow = config.values.grow = [](const LockFreeMapGrowContext&)
    {
        return size_t(2);
    };
    config.nodes.shrink = config.values.shrink = [&](const LockFreeMapShrinkContext& c)
    {
        if (c.initialCapacity != 2 || (c.successfulGrowCount == 0 && c.hasSuccessfulGrow))
        {
            contextErrors.fetch_add(1);
        }
        return LockFreeMapShrinkDecision(enabled.load() == 2 ? 2 : 100, enabled.load() == 0 ? 0 : 2);
    };
    if (map.lockFreeMapConfigure(config) != LOCK_FREE_MAP_OK)
    {
        return false;
    }
    for (int key = 0; key < 8; ++key)
    {
        if (!map.set(key, key))
        {
            return false;
        }
    }
    size_t released = 0;
    map.lockFreeMapMaintain(4096, released);
    if (map.lockFreeMapStats().nodes.capacity != 8 || map.lockFreeMapStats().nodes.retained != 8)
    {
        return false;
    }
    map.clear();
    map.lockFreeMapMaintain(4096, released);
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    map.lockFreeMapMaintain(4096, released);
    if (map.lockFreeMapStats().nodes.capacity != 8)
    {
        return false;
    }
    enabled.store(2);
    for (int pass = 0; pass < 12; ++pass)
    {
        map.lockFreeMapMaintain(4096, released);
        std::this_thread::sleep_for(std::chrono::milliseconds(3));
    }
    if (map.lockFreeMapStats().nodes.capacity != 2 || map.lockFreeMapStats().values.capacity != 2)
    {
        return false;
    }
    for (int key = 0; key < 6; ++key)
    {
        if (!map.set(key, key))
        {
            return false;
        }
    }
    map.clearAndFreeMemory();
    return contextErrors.load() == 0 && map.lockFreeMapStats().nodes.capacity == 0 &&
    map.lockFreeMapStats().values.bytes == 0 && map.set(1, 1);
}

bool LockFreeMapTests::stress()
{
    LockFreeMap<int, int> map;
    LockFreeMapConfig config;
    config.nodes.initialCapacity = config.values.initialCapacity = 128;
    config.nodes.maximumCapacity = config.values.maximumCapacity = 1024;
    if (map.lockFreeMapConfigure(config) != LOCK_FREE_MAP_OK)
    {
        return false;
    }
    std::atomic<int> errors(0), start(0);
    std::vector<LockFreeMapTestThread> threads;
    threads.reserve(8);
    try
    {
        for (int id = 0; id < 8; ++id)
        {
            threads.push_back(LockFreeMapTestThread([&, id]()
            {
                if (!wait(start))
                {
                    errors.fetch_add(1);
                    return;
                }
                uint32_t random = 314159 + id;
                for (int iteration = 0; iteration < 20000; ++iteration)
                {
                    random = random * 1664525u + 1013904223u;
                    const int key = (random >> 8) % 32;
                    const int operation = random % 6;
                    LockFreeMapStatus status = LOCK_FREE_MAP_OK;
                    if (operation < 2)
                    {
                        status = map.lockFreeMapSet(key, key * 7);
                    }
                    else if (operation == 2)
                    {
                        int value = -1;
                        status = map.lockFreeMapExtract(key, value);
                        if (status == LOCK_FREE_MAP_COMMITTED && value != key * 7)
                        {
                            errors.fetch_add(1);
                        }
                    }
                    else if (operation == 3)
                    {
                        map.clear();
                    }
                    else if (operation == 4)
                    {
                        int previous = -1;
                        map.forEach([&](const int& k, const int& v)
                        {
                            if (k <= previous || v != k * 7)
                            {
                                errors.fetch_add(1);
                            }
                            previous = k;
                        });
                    }
                    else
                    {
                        size_t released = 0;
                        status = map.lockFreeMapMaintain(64, released);
                    }
                    if (status != LOCK_FREE_MAP_OK && status != LOCK_FREE_MAP_COMMITTED &&
                        status != LOCK_FREE_MAP_NOT_FOUND)
                    {
                        errors.fetch_add(1);
                    }
                }
            }));
        }
    }
    catch (...)
    {
        errors.fetch_add(1);
    }
    start.store(1);
    for (size_t index = 0; index < threads.size(); ++index)
    {
        threads[index].join();
    }
    map.clear();
    size_t released = 0;
    for (int pass = 0; pass < 20; ++pass)
    {
        map.lockFreeMapMaintain(65536, released);
    }
    LockFreeMapStats stats = map.lockFreeMapStats();

    return errors.load() == 0 && stats.nodes.retained == 0 && stats.values.retained == 0 &&
    stats.indexBytes == 0 && stats.activeOperations == 0;
}

bool LockFreeMapTests::ownership()
{
    // 进程缓存：淘汰仅移除容器自身的拥有引用
    LockFreeMap<int, std::shared_ptr<int>> cache;
    std::shared_ptr<int> original(new int(42)), borrowed;
    std::weak_ptr<int> weak(original);
    LockFreeMapToken old;
    if (!cache.set(77, original) || cache.lockFreeMapGet(77, borrowed, old) != LOCK_FREE_MAP_OK)
    {
        return false;
    }
    original.reset();
    if (cache.lockFreeMapErase(77, old) != LOCK_FREE_MAP_COMMITTED || weak.expired())
    {
        return false;
    }
    size_t released = 0;
    cache.lockFreeMapMaintain(4096, released);
    if (weak.expired() || *borrowed != 42)
    {
        return false;
    }
    borrowed.reset();
    if (!weak.expired())
    {
        return false;
    }
    // 最小调度适配器：每个桶的重做槽优先于普通先进先出队列
    LockFreeMap<int, std::shared_ptr<std::vector<char>>> priorities;
    std::shared_ptr<std::vector<char>> low(new std::vector<char>), high(new std::vector<char>);
    low->push_back('A'); high->push_back('B'); high->push_back('C');
    low->push_back('D');
    if (!priorities.set(1, low) || !priorities.set(2, high))
    {
        return false;
    }
    std::string order;
    while (!priorities.empty())
    {
        int priority = 0;
        std::shared_ptr<std::vector<char>> bucket, selected;
        LockFreeMapToken token;
        if (priorities.lockFreeMapLast(priority, bucket, token) != LOCK_FREE_MAP_OK ||
            priorities.lockFreeMapExtractLast(token, priority, selected) != LOCK_FREE_MAP_COMMITTED)
        {
            return false;
        }
        for (size_t index = 0; index < selected->size(); ++index)
        {
            order += (*selected)[index];
        }
    }
    // A 是预先准备的重做任务副本，容器自身不执行任务中断
    return order == "BCAD";
}

bool LockFreeMapTests::allocations()
{
    const int64_t baseline = LockFreeMapTestFaults::m_outstanding.load();
    // 先证明非抛出分配确实进入跟踪器；不能仅凭容器场景间接推断
    for (int array = 0; array < 2; ++array)
    {
        LockFreeMapTestFaults::m_failAfter.store(0);
        void* failed = array == 0 ? ::operator new(31, std::nothrow) : ::operator new[](31, std::nothrow);
        LockFreeMapTestFaults::m_failAfter.store(-1);
        const bool rejected = failed == nullptr;
        if (array == 0)
        {
            ::operator delete(failed);
        }
        else
        {
            ::operator delete[](failed);
        }
        const uint64_t before = LockFreeMapTestFaults::m_allocations.load();
        void* memory = array == 0 ? ::operator new(31, std::nothrow) : ::operator new[](31, std::nothrow);
        const bool tracked = memory != nullptr && LockFreeMapTestFaults::m_allocations.load() == before + 1 &&
            LockFreeMapTestFaults::m_outstanding.load() == baseline + 1;
        if (array == 0)
        {
            ::operator delete(memory, std::nothrow);
        }
        else
        {
            ::operator delete[](memory, std::nothrow);
        }
        if (!rejected || !tracked || LockFreeMapTestFaults::m_outstanding.load() != baseline)
        {
            return false;
        }
    }
    int constructorFaults = 0, growFaults = 0, allocationFreeUpdates = 0, indexFaults = 0;
    for (int fail = 0; fail < 40; ++fail)
    {
        LockFreeMapTestFaults::m_failAfter.store(fail);
        try
        {
            LockFreeMap<int, int> map;
            LockFreeMapTestFaults::m_failAfter.store(-1);
            int value = 0;
            LockFreeMapToken token;
            const LockFreeMapStatus status = map.lockFreeMapGet(0, value, token);
            if (status == LOCK_FREE_MAP_ALLOCATION_FAILURE)
            {
                ++constructorFaults;
            }
            else if (status != LOCK_FREE_MAP_NOT_FOUND)
            {
                return false;
            }
        }
        catch (const std::bad_alloc&)
        {
            ++constructorFaults;
        }
        LockFreeMapTestFaults::m_failAfter.store(-1);
        if (LockFreeMapTestFaults::m_outstanding.load() != baseline)
        {
            return false;
        }
    }
    for (int fail = 0; fail < 20; ++fail)
    {
        LockFreeMap<int, int> map;
        LockFreeMapConfig config;
        config.nodes.initialCapacity = config.values.initialCapacity = 0;
        if (map.lockFreeMapConfigure(config) != LOCK_FREE_MAP_OK)
        {
            return false;
        }
        LockFreeMapTestFaults::m_failAfter.store(fail);
        LockFreeMapStatus status = map.lockFreeMapSet(1, 2);
        LockFreeMapTestFaults::m_failAfter.store(-1);
        if (status == LOCK_FREE_MAP_ALLOCATION_FAILURE)
        {
            ++growFaults;
        }
        else if (status != LOCK_FREE_MAP_COMMITTED)
        {
            return false;
        }
        if ((status != LOCK_FREE_MAP_COMMITTED && map.contains(1)) || !map.set(1, 2))
        {
            return false;
        }
        LockFreeMapTestFaults::m_failAfter.store(0);
        status = map.lockFreeMapSet(1, 3);
        LockFreeMapTestFaults::m_failAfter.store(-1);
        if (status != LOCK_FREE_MAP_COMMITTED || map.value(1) != 3)
        {
            return false;
        }
        ++allocationFreeUpdates;
        LockFreeMapTestFaults::m_failAfter.store(0);
        status = map.lockFreeMapSet(2, 4);
        LockFreeMapTestFaults::m_failAfter.store(-1);
        if (status != LOCK_FREE_MAP_ALLOCATION_FAILURE || map.contains(2) || map.value(1) != 3)
        {
            return false;
        }
        ++indexFaults;
    }
    const int64_t remaining = LockFreeMapTestFaults::m_outstanding.load();
    std::fprintf(LockFreeMapTestReport::m_file,
        "allocation_sweep,constructor=%d,grow=%d,allocation_free_updates=%d,index_failures=%d,before=%lld,after=%lld\n",
        constructorFaults, growFaults, allocationFreeUpdates, indexFaults,
        static_cast<long long>(baseline), static_cast<long long>(remaining));

    return constructorFaults >= 10 && growFaults >= 5 && allocationFreeUpdates == 20 && indexFaults == 20 && remaining == baseline;
}

bool LockFreeMapTests::limits()
{
    std::atomic<uint64_t> saturated((std::numeric_limits<uint64_t>::max)() - 1);
    LockFreeMapPool<int>::increment(saturated);
    LockFreeMapPool<int>::increment(saturated);
    if (saturated.load() != (std::numeric_limits<uint64_t>::max)())
    {
        return false;
    }
    LockFreeMap<int, int> map;
    LockFreeMapConfig config;
    config.nodes.maximumCapacity = -2;
    if (map.lockFreeMapConfigure(config) != LOCK_FREE_MAP_INVALID)
    {
        return false;
    }
    config.nodes.maximumCapacity = -1;
    config.operationRecordLimit = 1;
    if (map.lockFreeMapConfigure(config) != LOCK_FREE_MAP_OK || !map.set(1, 1))
    {
        return false;
    }
    bool limit = false;
    map.forEach([&](const int&, const int&)
    {
        int value = 0; LockFreeMapToken token;
        limit = map.lockFreeMapGet(1, value, token) == LOCK_FREE_MAP_RECORD_LIMIT;
    });
    if (!limit || !map.contains(1))
    {
        return false;
    }
    LockFreeMapIndexStore<int, int, std::less<int>> store(config);
    store.m_version.store((std::numeric_limits<uint64_t>::max)() - 1);
    uint64_t version = 0;
    if (!store.nextVersion(version) || version != (std::numeric_limits<uint64_t>::max)() || store.nextVersion(version))
    {
        return false;
    }
    LockFreeMapIndexStore<int, int, std::less<int>> ranged(config);
    LockFreeMapGuard rangeGuard(ranged.m_domain);
    ranged.m_version.store((std::numeric_limits<uint64_t>::max)() - 2);
    if (!ranged.nextVersion(version, rangeGuard) || version != (std::numeric_limits<uint64_t>::max)() - 1 ||
        !ranged.nextVersion(version, rangeGuard) || version != (std::numeric_limits<uint64_t>::max)() ||
        ranged.nextVersion(version, rangeGuard))
    {
        return false;
    }
    map.clearAndFreeMemory();
    config.nodes.initialCapacity = config.values.initialCapacity = 0;
    config.nodes.grow = [](const LockFreeMapGrowContext&)
    {
        return size_t(0);
    };
    if (map.lockFreeMapConfigure(config) != LOCK_FREE_MAP_OK || map.lockFreeMapSet(1, 1) != LOCK_FREE_MAP_GROW_DENIED)
    {
        return false;
    }
    config.nodes.grow = [](const LockFreeMapGrowContext&) -> size_t
    {
        throw std::runtime_error("policy");
    };
    if (map.lockFreeMapConfigure(config) != LOCK_FREE_MAP_OK || map.lockFreeMapSet(1, 1) != LOCK_FREE_MAP_POLICY_EXCEPTION)
    {
        return false;
    }
    config.nodes.grow = [](const LockFreeMapGrowContext&)
    {
        return (std::numeric_limits<size_t>::max)();
    };
    config.nodes.maximumCapacity = 3;
    if (map.lockFreeMapConfigure(config) != LOCK_FREE_MAP_OK || !map.set(1, 1) || map.lockFreeMapStats().nodes.capacity != 3)
    {
        return false;
    }
    map.clearAndFreeMemory();
    config.nodes.initialCapacity = 2;
    config.nodes.maximumCapacity = 4;
    config.nodes.growFreeThreshold = 1;
    if (map.lockFreeMapConfigure(config) != LOCK_FREE_MAP_OK || !map.set(1, 1) || map.lockFreeMapStats().nodes.capacity != 4)
    {
        return false;
    }
    map.clearAndFreeMemory();
    config.nodes.shrink = [](const LockFreeMapShrinkContext&) -> LockFreeMapShrinkDecision
    {
        throw std::runtime_error("shrink");
    };
    if (map.lockFreeMapConfigure(config) != LOCK_FREE_MAP_OK)
    {
        return false;
    }
    size_t released = 0;
    return map.lockFreeMapMaintain(64, released) == LOCK_FREE_MAP_POLICY_EXCEPTION && map.set(1, 1);
}

bool LockFreeMapTests::poolConcurrency()
{
    LockFreeMapReclamation domain(16);
    LockFreeMapPoolConfig config;
    config.initialCapacity = 0;
    config.maximumCapacity = 7;
    config.grow = [](const LockFreeMapGrowContext&)
    {
        return size_t(1000);
    };
    LockFreeMapPool<int> pool(config, domain);
    std::atomic<int> errors(0), start(0);
    std::vector<LockFreeMapTestThread> threads;
    threads.reserve(12);
    try
    {
        for (int id = 0; id < 12; ++id)
        {
            threads.push_back(LockFreeMapTestThread([&, id]()
            {
                LockFreeMapGuard guard(domain);
                if (!wait(start) || guard.m_record == nullptr)
                {
                    errors.fetch_add(1);
                    return;
                }
                for (int step = 0; step < 10000; ++step)
                {
                    LockFreeMapStatus status;
                    LockFreeMapPoolSlot<int>* slot = pool.create(id, 0, guard, status);
                    if (slot != nullptr)
                    {
                        if (slot->value() != id)
                        {
                            errors.fetch_add(1);
                        }
                        pool.release(slot);
                    }
                    else if (status != LOCK_FREE_MAP_CAPACITY && status != LOCK_FREE_MAP_RESERVED && status != LOCK_FREE_MAP_CONFLICT)
                    {
                        errors.fetch_add(1);
                    }
                    LockFreeMapPoolStats stats = pool.stats();
                    if (stats.capacity + stats.reserved > 7)
                    {
                        errors.fetch_add(1);
                    }
                }
            }));
        }
    }
    catch (...)
    {
        errors.fetch_add(1);
    }
    start.store(1);
    for (size_t index = 0; index < threads.size(); ++index)
    {
        threads[index].join();
    }
    return errors.load() == 0 && pool.stats().capacity == 7 && pool.stats().reserved == 0 && pool.stats().retained == 0;
}

bool LockFreeMapTests::pausedReclaimer()
{
    LockFreeMap<int, LockFreeMapTestValue> map;
    if (!map.set(1, LockFreeMapTestValue(199)))
    {
        return false;
    }
    LockFreeMapTestValue copied;
    copied.m_pauseAssignment = true;
    LockFreeMapTestValue::m_entered.store(0);
    LockFreeMapTestValue::m_release.store(0);
    LockFreeMapTestThread reader([&]() { map.get(1, copied); });
    bool prepared = wait(LockFreeMapTestValue::m_entered);
    prepared = map.set(1, LockFreeMapTestValue(1)) && prepared;
    LockFreeMapTestValue::m_release.store(1);
    reader.join();
    if (!prepared)
    {
        return false;
    }
    LockFreeMapTestValue::m_entered.store(0);
    LockFreeMapTestValue::m_release.store(0);
    LockFreeMapTestValue::m_pauseDestructorValue.store(199);
    LockFreeMapTestThread reclaimer([&]()
    {
        size_t released = 0; map.lockFreeMapMaintain(4096, released);
    });
    bool ok = wait(LockFreeMapTestValue::m_entered);
    for (int iteration = 0; iteration < 2000 && ok; ++iteration)
    {
        ok = map.set(2, LockFreeMapTestValue(2));
    }
    LockFreeMapTestValue::m_release.store(1);
    reclaimer.join();
    LockFreeMapTestValue::m_pauseDestructorValue.store(-1);
    size_t released = 0;
    map.lockFreeMapMaintain(4096, released);
    return ok && map.size() == 2 && map.retainedValueCount() == 2;
}

bool LockFreeMapTests::detachedDestruction()
{
    const int before = LockFreeMapTestValue::m_live.load();
    const size_t timeouts = LockFreeMapTestValue::m_timeouts.load();
    bool ok = false;
    {
        LockFreeMap<int, LockFreeMapTestValue> map;
        LockFreeMapConfig config;
        LockFreeMapTestValue output;
        LockFreeMapToken oldToken;
        std::atomic<bool> erased(false);
        LockFreeMapTestThread remover;
        try
        {
            config.nodes.initialCapacity = config.values.initialCapacity = 3;
            // 保持无上限删除语义，但策略拒绝增长，独立检验正在析构的槽不可复用
            config.nodes.grow = [](const LockFreeMapGrowContext& context) -> size_t
            {
                (void)context;
                return 0;
            };
            config.values.grow = config.nodes.grow;
            ok = map.lockFreeMapConfigure(config) == LOCK_FREE_MAP_OK &&
                map.set(1, LockFreeMapTestValue(199)) && map.set(2, LockFreeMapTestValue(200)) &&
                map.lockFreeMapGet(1, output, oldToken) == LOCK_FREE_MAP_OK;
            if (ok)
            {
                LockFreeMapTestValue::m_entered.store(0);
                LockFreeMapTestValue::m_release.store(0);
                LockFreeMapTestValue::m_pauseDestructorValue.store(199);
                remover = LockFreeMapTestThread([&]()
                {
                    erased.store(map.erase(1));
                });
                ok = wait(LockFreeMapTestValue::m_entered);
                ok = map.size() == 1 && !map.contains(1) && ok;
                // 一个槽暂停析构，另两个值槽仍须支持重复覆盖，不能等待删除线程
                for (int iteration = 0; iteration < 1000 && ok; ++iteration)
                {
                    ok = map.set(2, LockFreeMapTestValue(1000 + iteration));
                }
                ok = map.set(3, LockFreeMapTestValue(333)) && ok;
                const LockFreeMapStatus rejected = map.lockFreeMapSet(4, LockFreeMapTestValue(444));
                ok = rejected == LOCK_FREE_MAP_GROW_DENIED && !map.contains(4) && ok;
                const LockFreeMapStats stats = map.lockFreeMapStats();
                ok = stats.nodes.capacity == 3 && stats.values.capacity == 3 &&
                    stats.nodes.reserved == 0 && stats.values.reserved == 0 && ok;
            }
        }
        catch (...)
        {
            ok = false;
        }
        // 任何失败都先解除暂停再回收线程，避免异常收尾等待仍关闭的测试门
        LockFreeMapTestValue::m_release.store(1);
        const bool joined = remover.join();
        LockFreeMapTestValue::m_pauseDestructorValue.store(-1);
        ok = joined && erased.load() && ok;
        try
        {
            ok = map.set(1, LockFreeMapTestValue(777)) && ok;
            ok = map.lockFreeMapErase(1, oldToken) == LOCK_FREE_MAP_CONFLICT && ok;
            ok = map.get(1, output) && output.m_value == 777 && map.size() == 3 && ok;
            map.clearAndFreeMemory();
            ok = map.retainedNodeCount() == 0 && map.retainedValueCount() == 0 && ok;
        }
        catch (...)
        {
            ok = false;
        }
    }
    return ok && LockFreeMapTestValue::m_live.load() == before &&
        LockFreeMapTestValue::m_timeouts.load() == timeouts;
}

bool LockFreeMapTests::historyBound()
{
    LockFreeMap<int, int> map;
    LockFreeMapConfig config;
    config.nodes.initialCapacity = config.values.initialCapacity = 4;
    config.nodes.maximumCapacity = config.values.maximumCapacity = 4;
    if (map.lockFreeMapConfigure(config) != LOCK_FREE_MAP_OK)
    {
        return false;
    }
    const size_t metadata = map.lockFreeMapStats().metadataBytes;
    for (int key = 0; key < 100000; ++key)
    {
        if (!map.set(key, key) || !map.erase(key))
        {
            return false;
        }
        if (map.retainedNodeCount() > 2 || map.retainedValueCount() > 2 ||
            map.lockFreeMapStats().metadataBytes != metadata)
        {
            return false;
        }
    }
    size_t released = 0;
    map.lockFreeMapMaintain(4096, released);
    return map.retainedNodeCount() == 0 && map.retainedValueCount() == 0 && map.lockFreeMapStats().indexBytes == 0;
}

bool LockFreeMapTests::smallBudget()
{
    LockFreeMap<int, LockFreeMapTestValue> map;
    LockFreeMapConfig config;
    config.operationRecordLimit = 2;
    config.maintenanceBudget = 1;
    config.nodes.initialCapacity = 16;
    config.nodes.maximumCapacity = 16;
    config.values.initialCapacity = 32;
    config.values.maximumCapacity = 32;
    if (map.lockFreeMapConfigure(config) != LOCK_FREE_MAP_OK)
    {
        return false;
    }
    for (int key = 0; key < 8; ++key)
    {
        if (!map.set(key, LockFreeMapTestValue(key)))
        {
            return false;
        }
    }
    LockFreeMapTestValue output;
    output.m_pauseAssignment = true;
    LockFreeMapTestValue::m_entered.store(0);
    LockFreeMapTestValue::m_release.store(0);
    bool read = false;
    // 读者在复制旧值时持有旧根；清空和维护不能释放它正在借用的对象
    LockFreeMapTestThread reader([&]()
    {
        read = map.get(0, output);
    });
    bool passed = wait(LockFreeMapTestValue::m_entered);
    map.clear();
    passed = map.set(100, LockFreeMapTestValue(100)) && passed;
    size_t released = 0;
    for (size_t step = 0; step < 32; ++step)
    {
        passed = map.lockFreeMapMaintain(1, released) == LOCK_FREE_MAP_OK && passed;
    }
    passed = passed && map.retainedNodeCount() >= 9 && map.retainedValueCount() >= 9;
    LockFreeMapTestValue::m_release.store(1);
    reader.join();
    passed = passed && read && output.m_value == 0;
    for (size_t step = 0; step < 128; ++step)
    {
        passed = map.lockFreeMapMaintain(1, released) == LOCK_FREE_MAP_OK && passed;
    }
    passed = passed && map.size() == 1 && map.retainedNodeCount() == 1 && map.retainedValueCount() == 1;
    map.clear();
    for (size_t step = 0; step < 128; ++step)
    {
        passed = map.lockFreeMapMaintain(1, released) == LOCK_FREE_MAP_OK && passed;
    }
    return passed && map.retainedNodeCount() == 0 && map.retainedValueCount() == 0 &&
        map.lockFreeMapStats().indexBytes == 0;
}

bool LockFreeMapTests::pausedGrowth()
{
    LockFreeMap<int, int> map;
    LockFreeMapConfig config;
    config.nodes.initialCapacity = 1;
    config.nodes.maximumCapacity = 1;
    config.values.initialCapacity = 0;
    config.values.maximumCapacity = 1;
    if (map.lockFreeMapConfigure(config) != LOCK_FREE_MAP_OK)
    {
        return false;
    }
    LockFreeMapTestValue::m_entered.store(0);
    LockFreeMapTestValue::m_release.store(0);
    LockFreeMapStatus status = LOCK_FREE_MAP_INVALID;
    LockFreeMapTestThread allocator([&]()
    {
        LockFreeMapTestFaults::m_pauseAllocation.store(0);
        status = map.lockFreeMapSet(1, 1);
    });
    bool ok = wait(LockFreeMapTestValue::m_entered);
    const LockFreeMapStats pending = map.lockFreeMapStats();
    ok = ok && pending.values.capacity == 0 && pending.values.reserved == 1 &&
        map.lockFreeMapSet(2, 2) == LOCK_FREE_MAP_RESERVED && !map.contains(1);
    map.clear();
    LockFreeMapTestValue::m_release.store(1);
    allocator.join();
    LockFreeMapTestFaults::m_pauseAllocation.store(-1);
    return ok && status == LOCK_FREE_MAP_COMMITTED && map.value(1, -1) == 1 &&
        map.lockFreeMapStats().values.capacity == 1 && map.lockFreeMapStats().values.reserved == 0;
}

bool LockFreeMapTests::resources(size_t& threads, size_t& handles)
{
    threads = 0;
    handles = 0;
#ifdef _WIN32
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
    {
        return false;
    }
    THREADENTRY32 entry = {};
    entry.dwSize = sizeof(entry);
    BOOL found = Thread32First(snapshot, &entry);
    const DWORD process = GetCurrentProcessId();
    while (found)
    {
        if (entry.th32OwnerProcessID == process)
        {
            ++threads;
        }
        found = Thread32Next(snapshot, &entry);
    }
    const DWORD error = GetLastError();
    const BOOL closed = CloseHandle(snapshot);
    DWORD count = 0;
    if (error != ERROR_NO_MORE_FILES || !closed || !GetProcessHandleCount(GetCurrentProcess(), &count))
    {
        return false;
    }
    handles = count;
#endif
    return true;
}

bool LockFreeMapTests::retiredVersionRace()
{
    LockFreeMap<int, LockFreeMapTestValue> map;
    LockFreeMapTestValue original(1), replacement(2), output;
    if (!map.set(1, original))
    {
        return false;
    }
    LockFreeMapToken token;
    int key = 0;
    if (map.lockFreeMapLast(key, output, token) != LOCK_FREE_MAP_OK)
    {
        return false;
    }
    output.m_pauseAssignment = true;
    LockFreeMapTestValue::m_entered.store(0);
    LockFreeMapTestValue::m_release.store(0);
    LockFreeMapStatus extracted = LOCK_FREE_MAP_INVALID;
    LockFreeMapTestThread reader([&]() { extracted = map.lockFreeMapExtractLast(token, key, output); });
    bool ok = wait(LockFreeMapTestValue::m_entered);
    ok = map.set(1, replacement) && ok;
    LockFreeMapTestValue::m_release.store(1);
    reader.join();
    output.m_pauseAssignment = false;
    size_t released = 0;
    map.lockFreeMapMaintain(4096, released);
    return ok && extracted == LOCK_FREE_MAP_CONFLICT && map.get(1, output) &&
        output.m_value == 2 && map.size() == 1 && map.retainedValueCount() == 1;
}

bool LockFreeMapTests::descriptorHelping()
{
    for (int changed = 0; changed != 2; ++changed)
    {
        LockFreeMapConfig config;
        LockFreeMapIndexStore<int, int, std::less<int>> store(config);
        if (store.m_nodes.initialize() != LOCK_FREE_MAP_OK || store.m_values.initialize() != LOCK_FREE_MAP_OK)
        {
            return false;
        }
        LockFreeMapGuard producer(store.m_domain);
        LockFreeMapStatus status = LOCK_FREE_MAP_INVALID;
        LockFreeMapPoolSlot<int>* first = store.m_values.create(11, 1, producer, status);
        if (first == nullptr)
        {
            return false;
        }
        LockFreeMapPoolSlot<LockFreeMapKey<int, int>>* node = store.m_nodes.create(
            LockFreeMapKey<int, int>(1, store.m_values), 0, producer, status);
        if (node == nullptr)
        {
            store.m_values.release(first);
            return false;
        }
        node->mutableValue().m_value.store(first);
        LockFreeMapIndex<int, int>* initial = store.build(nullptr, 0, false, node, nullptr, producer, status);
        if (initial == nullptr)
        {
            store.m_nodes.release(node);
            return false;
        }
        store.m_root.store(initial);
        store.complete(initial, nullptr, producer);
        store.protect(producer);
        store.protectValue(node, producer);
        LockFreeMapIndex<int, int>* pending = store.build(initial, 0, true, nullptr, first, producer, status);
        if (pending == nullptr)
        {
            return false;
        }
        if (changed != 0)
        {
            // 模拟写者在待定删除发布前通过根校验，随后赢得值 CAS
            // 验证旧删除计划不能误删该替换版本
            LockFreeMapPoolSlot<int>* second = store.m_values.create(22, 2, producer, status);
            if (second == nullptr)
            {
                store.discard(pending);
                return false;
            }
            node->mutableValue().m_value.store(second);
            store.m_values.release(first);
        }
        store.m_root.store(pending);
        bool helped = false;
        LockFreeMapTestThread helper([&]()
        {
            LockFreeMapGuard observer(store.m_domain);
            LockFreeMapIndex<int, int>* view = store.protect(observer);
            helped = pending->m_state.load() == 1 && view == pending &&
                view->count() == static_cast<size_t>(changed);
            if (changed != 0)
            {
                LockFreeMapPoolSlot<int>* value = store.protectValue(view->entry(0).m_key, observer);
                helped = helped && value != nullptr && value->value() == 22 && value->m_version == 2;
            }
        });
        helper.join();
        store.complete(pending, initial, producer);
        if (!helped || store.help(pending) != static_cast<uint32_t>(changed != 0 ? 3 : 2))
        {
            return false;
        }
        // 清空可以摘除任一已完成结果
        // 延迟帮助者此后不得恢复原根或待定根
        store.m_root.store(nullptr);
        store.help(pending);
        if (store.m_root.load() != nullptr)
        {
            return false;
        }
    }
    return true;
}

bool LockFreeMapTests::fastUpdates()
{
    LockFreeMap<int, int> map;
    LockFreeMapConfig config;
    config.nodes.initialCapacity = 1;
    config.nodes.maximumCapacity = 1;
    config.values.initialCapacity = 16;
    if (map.lockFreeMapConfigure(config) != LOCK_FREE_MAP_OK || !map.set(1, 0))
    {
        return false;
    }
    LockFreeMapToken original;
    int value = -1;
    if (map.lockFreeMapGet(1, value, original) != LOCK_FREE_MAP_OK)
    {
        return false;
    }
    bool ok = true;
    LockFreeMapTestFaults::m_failAfter.store(0);
    for (int iteration = 1; iteration <= 10000; ++iteration)
    {
        if (!map.set(1, iteration) || !map.get(1, value) || value != iteration)
        {
            ok = false;
            break;
        }
    }
    const bool allocationAttempted = LockFreeMapTestFaults::m_failAfter.load() != 0;
    LockFreeMapTestFaults::m_failAfter.store(-1);
    if (!ok || allocationAttempted || map.lockFreeMapUpdate(1, original, -1) != LOCK_FREE_MAP_CONFLICT || !map.set(1, 0))
    {
        return false;
    }
    std::atomic<int> start(0), errors(0);
    std::vector<LockFreeMapTestThread> threads;
    threads.reserve(4);
    try
    {
        for (int worker = 0; worker < 4; ++worker)
        {
            threads.push_back(LockFreeMapTestThread([&]()
            {
                if (!wait(start))
                {
                    errors.fetch_add(1);
                    return;
                }
                for (int iteration = 0; iteration < 2000; ++iteration)
                {
                    for (;;)
                    {
                        int previous = -1;
                        LockFreeMapToken token;
                        if (map.lockFreeMapGet(1, previous, token) != LOCK_FREE_MAP_OK)
                        {
                            errors.fetch_add(1);
                            break;
                        }
                        const LockFreeMapStatus status = map.lockFreeMapUpdate(1, token, previous + 1);
                        if (status == LOCK_FREE_MAP_COMMITTED)
                        {
                            break;
                        }
                        if (status != LOCK_FREE_MAP_CONFLICT)
                        {
                            errors.fetch_add(1);
                            break;
                        }
                    }
                }
            }));
        }
    }
    catch (...)
    {
        errors.fetch_add(1);
    }
    start.store(1);
    for (size_t index = 0; index < threads.size(); ++index)
    {
        threads[index].join();
    }
    size_t released = 0;
    map.lockFreeMapMaintain(4096, released);
    const bool found = map.get(1, value);

    return errors.load() == 0 && found && value == 8000 && map.retainedValueCount() == 1;
}

bool LockFreeMapTests::directKeyConstruction()
{
    LockFreeMap<LockFreeMapTestCopyKey, int> map;
    LockFreeMapTestCopyKey first(1), second(2);
    LockFreeMapTestCopyKey::m_copies.store(0);
    if (!map.set(first, 7) || LockFreeMapTestCopyKey::m_copies.load() != 1 || !map.set(first, 8) ||
        LockFreeMapTestCopyKey::m_copies.load() != 1)
    {
        return false;
    }
    LockFreeMapTestCopyKey::m_throw = true;
    const LockFreeMapStatus failed = map.lockFreeMapSet(second, 9);
    LockFreeMapTestCopyKey::m_throw = false;
    int value = 0;
    const LockFreeMapStats stats = map.lockFreeMapStats();
    if (failed != LOCK_FREE_MAP_USER_EXCEPTION || map.contains(second) || !map.get(first, value) || value != 8 ||
        stats.nodes.retained != 1 || stats.values.retained != 1 || !map.set(second, 9))
    {
        return false;
    }
    map.clearAndFreeMemory();
    if (!map.set(first, 7))
    {
        return false;
    }
    LockFreeMapTestCopyKey::m_copies.store(0);
    const std::vector<std::pair<LockFreeMapTestCopyKey, int>> snapshot = map.toVector();
    if (snapshot.size() != 1 || snapshot[0].first.m_value != 1 || snapshot[0].second != 7 ||
        LockFreeMapTestCopyKey::m_copies.load() != 1)
    {
        return false;
    }
    map.clearAndFreeMemory();
    return map.retainedNodeCount() == 0 && map.retainedValueCount() == 0 && map.lockFreeMapStats().indexBytes == 0;
}

bool LockFreeMapTests::sharedIndex()
{
    for (int bounded = 0; bounded < 2; ++bounded)
    {
        LockFreeMap<int, int> map;
        LockFreeMapConfig config;
        config.nodes.initialCapacity = config.nodes.maximumCapacity = 128;
        config.values.initialCapacity = 256;
        config.values.maximumCapacity = bounded == 0 ? 256 : -1;
        if (map.lockFreeMapConfigure(config) != LOCK_FREE_MAP_OK)
        {
            return false;
        }
        int expected[128];
        for (int key = 0; key < 128; ++key)
        {
            expected[key] = key;
            if (!map.set(key, key))
            {
                return false;
            }
        }
        LockFreeMapToken old;
        int value = -1;
        if (map.lockFreeMapGet(7, value, old) != LOCK_FREE_MAP_OK)
        {
            return false;
        }
        const uint64_t allocations = LockFreeMapTestFaults::m_allocations.load();
        LockFreeMapTestFaults::m_failAfter.store(0);
        bool passed = true;
        for (int iteration = 0; iteration < 10000; ++iteration)
        {
            const int key = iteration % 128;
            expected[key] = iteration + 1000;
            passed = map.erase(key) && map.set(key, expected[key]) && passed;
        }
        LockFreeMapTestFaults::m_failAfter.store(-1);
        passed = passed && allocations == LockFreeMapTestFaults::m_allocations.load();
        for (int key = 0; key < 128; ++key)
        {
            passed = map.get(key, value) && value == expected[key] && passed;
        }
        passed = map.lockFreeMapErase(7, old) == LOCK_FREE_MAP_CONFLICT && passed;
        passed = map.erase(127) && passed;
        int key = -1;
        LockFreeMapToken last;
        passed = map.lockFreeMapLast(key, value, last) == LOCK_FREE_MAP_OK && key == 126 && passed;
        size_t released = 0;
        for (int pass = 0; pass < 512; ++pass)
        {
            passed = map.lockFreeMapMaintain(1, released) == LOCK_FREE_MAP_OK && passed;
        }
        if (!passed || map.retainedNodeCount() != 127 || map.retainedValueCount() != 127 ||
            !map.set(1000, 1000) || map.size() != 128)
        {
            return false;
        }
    }
    LockFreeMapConfig config;
    LockFreeMapIndexStore<int, int, std::less<int>> store(config);
    if (store.m_nodes.initialize() != LOCK_FREE_MAP_OK || store.m_values.initialize() != LOCK_FREE_MAP_OK)
    {
        return false;
    }
    LockFreeMapGuard producer(store.m_domain);
    LockFreeMapStatus status = LOCK_FREE_MAP_INVALID;
    for (int key = 1; key <= 2; ++key)
    {
        LockFreeMapPoolSlot<int>* value = store.m_values.create(key * 11, key, producer, status);
        if (value == nullptr)
        {
            return false;
        }
        auto* node = store.m_nodes.emplace(0, producer, status, key, store.m_values);
        if (node == nullptr)
        {
            store.m_values.release(value);
            return false;
        }
        node->mutableValue().m_value.store(value);
        auto* source = store.protect(producer);
        auto* next = store.build(source, key - 1, false, node, nullptr, producer, status);
        if (next == nullptr)
        {
            store.m_nodes.release(node);
            return false;
        }
        store.m_root.store(next);
        store.complete(next, source, producer);
    }
    LockFreeMapGuard reader(store.m_domain);
    auto* oldView = store.protect(reader);
    auto* source = store.protect(producer);
    // 读者保持旧数组时删除首项并在同一排序位置插入不同键；旧视图不能被补写改变
    auto* removed = store.build(source, 0, true, nullptr, nullptr, producer, status, true);
    if (removed == nullptr)
    {
        return false;
    }
    store.m_root.store(removed);
    store.complete(removed, source, producer);
    source = store.protect(producer);
    auto* replacementValue = store.m_values.create(99, 9, producer, status);
    if (replacementValue == nullptr)
    {
        return false;
    }
    auto* replacement = store.m_nodes.emplace(0, producer, status, 0, store.m_values);
    if (replacement == nullptr)
    {
        store.m_values.release(replacementValue);
        return false;
    }
    replacement->mutableValue().m_value.store(replacementValue);
    auto* replaced = store.build(source, 0, false, replacement, nullptr, producer, status);
    if (replaced == nullptr)
    {
        store.m_nodes.release(replacement);
        return false;
    }
    store.m_root.store(replaced);
    store.complete(replaced, source, producer);
    if (oldView->entry(0).m_key->value().m_key != 1 || replaced->entry(0).m_key->value().m_key != 0 ||
        replaced->m_merged.load() != 0)
    {
        return false;
    }
    source = store.protect(producer);
    auto* value = store.m_values.create(33, 3, producer, status);
    if (value == nullptr)
    {
        return false;
    }
    auto* added = store.m_nodes.emplace(0, producer, status, 3, store.m_values);
    if (added == nullptr)
    {
        store.m_values.release(value);
        return false;
    }
    added->mutableValue().m_value.store(value);
    auto* pending = store.build(source, 2, false, added, nullptr, producer, status);
    if (pending == nullptr)
    {
        store.m_nodes.release(added);
        return false;
    }
    store.m_root.store(pending);
    bool progressed = false;
    // 发布者尚未完成引用转交时暂停；另一个调用者必须可从该视图继续发布
    // 工作结果在 join 后读取，输入对象始终由测试作用域和源保护保持存活
    LockFreeMapTestThread helper([&]()
    {
        LockFreeMapGuard guard(store.m_domain);
        auto* view = store.protect(guard);
        LockFreeMapStatus result = LOCK_FREE_MAP_INVALID;
        auto* nextValue = store.m_values.create(44, 4, guard, result);
        if (nextValue == nullptr)
        {
            return;
        }
        auto* node = store.m_nodes.emplace(0, guard, result, 4, store.m_values);
        if (node == nullptr)
        {
            store.m_values.release(nextValue);
            return;
        }
        node->mutableValue().m_value.store(nextValue);
        auto* next = store.build(view, 3, false, node, nullptr, guard, result);
        if (next == nullptr)
        {
            store.m_nodes.release(node);
            return;
        }
        const bool privateSource = view == pending && view->m_state.load() == 1;
        auto* expected = view;
        if (store.m_root.compare_exchange_strong(expected, next))
        {
            store.complete(next, view, guard);
            progressed = privateSource && next->count() == 4 && next->entry(2).m_key->value().m_key == 3;
        }
        else
        {
            store.discard(next);
        }
    });
    helper.join();
    const uint64_t before = LockFreeMapTestFaults::m_allocations.load();
    LockFreeMapTestFaults::m_failAfter.store(0);
    store.complete(pending, source, producer);
    LockFreeMapTestFaults::m_failAfter.store(-1);
    if (!progressed || before != LockFreeMapTestFaults::m_allocations.load() ||
        oldView->count() != 2 || oldView->entry(1).m_key->value().m_key != 2)
    {
        return false;
    }
    source = store.protect(producer);
    value = store.m_values.create(55, 5, producer, status);
    if (value == nullptr)
    {
        return false;
    }
    added = store.m_nodes.emplace(0, producer, status, 5, store.m_values);
    if (added == nullptr)
    {
        store.m_values.release(value);
        return false;
    }
    added->mutableValue().m_value.store(value);
    auto* discarded = store.build(source, 4, false, added, nullptr, producer, status);
    if (discarded == nullptr)
    {
        store.m_nodes.release(added);
        return false;
    }
    store.m_root.store(nullptr);
    auto* expected = source;
    const bool wronglyPublished = store.m_root.compare_exchange_strong(expected, discarded);
    if (wronglyPublished)
    {
        store.complete(discarded, source, producer);
        return false;
    }
    store.discard(discarded);
    reader.m_record->m_pointer[0].store(nullptr);
    producer.m_record->m_pointer[0].store(nullptr);
    size_t released = 0;
    for (int pass = 0; pass < 512; ++pass)
    {
        store.collect(1, nullptr, true);
        store.m_nodes.maintain(1, released);
        store.m_values.maintain(1, released);
    }
    return store.m_root.load() == nullptr && store.m_nodes.stats().retained == 0 &&
        store.m_values.stats().retained == 0 && store.m_indexBytes.load() == 0;
}

bool LockFreeMapTests::slotCache()
{
    // 清空源保护不代表归还操作记录，重入者不能提前取得仍在使用的私有缓存
    {
        LockFreeMapReclamation single(1);
        int protectedValue = 0;
        {
            LockFreeMapGuard owner(single);
            if (owner.m_record == nullptr || single.active() != 1)
            {
                return false;
            }
            owner.m_record->m_pointer[0].store(&protectedValue);
            if (!single.protects(&protectedValue))
            {
                return false;
            }
            owner.m_record->m_pointer[0].store(nullptr);
            LockFreeMapGuard nested(single);
            if (nested.m_record != nullptr || single.active() != 1 || single.protects(&protectedValue))
            {
                return false;
            }
        }
        LockFreeMapGuard reused(single);
        if (reused.m_record == nullptr || single.active() != 1)
        {
            return false;
        }
    }
    LockFreeMapReclamation domain(4);
    LockFreeMapPoolConfig config;
    config.initialCapacity = config.maximumCapacity = 1;
    LockFreeMapPool<LockFreeMapTestValue> pool(config, domain, true);
    LockFreeMapPool<LockFreeMapTestValue> other(config, domain, true);
    if (pool.initialize() != LOCK_FREE_MAP_OK || other.initialize() != LOCK_FREE_MAP_OK)
    {
        return false;
    }
    const int alive = LockFreeMapTestValue::m_live.load();
    LockFreeMapPoolSlot<LockFreeMapTestValue>* cached = nullptr;
    {
        LockFreeMapGuard owner(domain);
        LockFreeMapStatus status = LOCK_FREE_MAP_INVALID;
        cached = pool.emplace(1, owner, status, 11);
        if (cached == nullptr)
        {
            return false;
        }
        pool.release(cached, owner.m_record);
        if (LockFreeMapTestValue::m_live.load() != alive || pool.stats().retained != 0 ||
            owner.m_record->m_poolCache[1] != cached || domain.takeIdleCache(&pool, 1) != nullptr)
        {
            return false;
        }
        bool progressed = false;
        // 主线程停在持有空槽的调用内；其他线程须返回明确容量状态，不能等它退出
        LockFreeMapTestThread contender([&]()
        {
            LockFreeMapGuard guard(domain);
            LockFreeMapStatus result = LOCK_FREE_MAP_INVALID;
            auto* value = pool.emplace(2, guard, result, 22);
            progressed = value == nullptr && result == LOCK_FREE_MAP_CAPACITY;
            pool.release(value, guard.m_record);
        });
        contender.join();
        if (!progressed)
        {
            return false;
        }
        // 同域另一池不能把本池的原始槽当作自己的容量
        auto* different = other.emplace(3, owner, status, 33);
        if (different == nullptr || different == cached || different->value().m_value != 33)
        {
            other.release(different);
            return false;
        }
        other.release(different, owner.m_record);
    }
    // 从已返回的操作记录转取满池缓存，不扩大容量也不执行对象复制
    void* moved = domain.takeIdleCache(&pool, 1);
    if (moved != cached || domain.takeIdleCache(&pool, 1) != nullptr)
    {
        return false;
    }
    {
        LockFreeMapGuard next(domain);
        next.m_record->m_poolOwner[1] = &pool;
        next.m_record->m_poolCache[1] = moved;
        LockFreeMapStatus status = LOCK_FREE_MAP_INVALID;
        auto* value = pool.emplace(4, next, status, 44);
        if (value != cached || value->m_version != 4 || value->value().m_value != 44 || pool.stats().capacity != 1)
        {
            pool.release(value);
            return false;
        }
        pool.release(value, next.m_record);
        LockFreeMapTestValue candidate(55);
        LockFreeMapTestValue::m_throwCopy.store(1);
        value = pool.create(candidate, 5, next, status);
        LockFreeMapTestValue::m_throwCopy.store(0);
        if (value != nullptr || status != LOCK_FREE_MAP_USER_EXCEPTION || pool.stats().retained != 0)
        {
            pool.release(value);
            return false;
        }
        value = pool.emplace(6, next, status, 66);
        if (value == nullptr || value->value().m_value != 66)
        {
            pool.release(value);
            return false;
        }
        pool.release(value, next.m_record);
    }
    pool.freeMemory();
    other.freeMemory();
    if (LockFreeMapTestValue::m_live.load() != alive || pool.stats().capacity != 0)
    {
        return false;
    }
    {
        LockFreeMapGuard rebuilt(domain);
        LockFreeMapStatus status = LOCK_FREE_MAP_INVALID;
        auto* value = pool.emplace(7, rebuilt, status, 77);
        if (value == nullptr || value->value().m_value != 77 || value->m_version != 7)
        {
            pool.release(value);
            return false;
        }
        pool.release(value, rebuilt.m_record);
    }
    size_t released = 0;
    pool.maintain(4, released);
    return LockFreeMapTestValue::m_live.load() == alive && pool.stats().retained == 0 && pool.stats().reusable == 1;
}