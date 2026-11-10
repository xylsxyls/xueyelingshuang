#include "LockFreeMapHistorical.h"
#include "LockFreeMapTest.h"
#include "LockFreeMapTestThread.h"
#include "LockFreeMap/LockFreeMapAPI.h"
#include <atomic>
#include <functional>
#include <stdint.h>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

bool LockFreeMapHistoricalFaults::m_keyCopy = false;
bool LockFreeMapHistoricalFaults::m_valueCopy = false;
bool LockFreeMapHistoricalFaults::m_valueAssign = false;
bool LockFreeMapHistoricalFaults::m_less = false;

LockFreeMapHistoricalKey::LockFreeMapHistoricalKey(int32_t value) :
m_value(value)
{

}

LockFreeMapHistoricalKey::LockFreeMapHistoricalKey(const LockFreeMapHistoricalKey& other) :
m_value(other.m_value)
{
    if (LockFreeMapHistoricalFaults::m_keyCopy)
    {
        throw std::runtime_error("LockFreeMapHistoricalKey copy failed");
    }
}

LockFreeMapHistoricalKey& LockFreeMapHistoricalKey::operator=(const LockFreeMapHistoricalKey& other)
{
    if (this == &other)
    {
        return *this;
    }
    if (LockFreeMapHistoricalFaults::m_keyCopy)
    {
        throw std::runtime_error("LockFreeMapHistoricalKey assign failed");
    }
    m_value = other.m_value;
    return *this;
}

LockFreeMapHistoricalValue::LockFreeMapHistoricalValue(int32_t value) :
m_value(value)
{

}

LockFreeMapHistoricalValue::LockFreeMapHistoricalValue(const LockFreeMapHistoricalValue& other) :
m_value(other.m_value)
{
    if (LockFreeMapHistoricalFaults::m_valueCopy)
    {
        throw std::runtime_error("LockFreeMapHistoricalValue copy failed");
    }
}

LockFreeMapHistoricalValue& LockFreeMapHistoricalValue::operator=(const LockFreeMapHistoricalValue& other)
{
    if (this == &other)
    {
        return *this;
    }
    if (LockFreeMapHistoricalFaults::m_valueAssign)
    {
        throw std::runtime_error("LockFreeMapHistoricalValue assign failed");
    }
    m_value = other.m_value;
    return *this;
}

bool LockFreeMapHistoricalLess::operator()(const LockFreeMapHistoricalKey& left, const LockFreeMapHistoricalKey& right) const
{
    if (LockFreeMapHistoricalFaults::m_less)
    {
        throw std::runtime_error("LockFreeMapHistoricalLess failed");
    }
    return left.m_value < right.m_value;
}

int LockFreeMapHistorical()
{
    int32_t totalCount = 0;
    int32_t failCount = 0;
    std::function<void(bool, const std::string&)> check = [&totalCount, &failCount](bool ok, const std::string& name) -> void
    {
        ++totalCount;
        std::fprintf(LockFreeMapTestReport::m_file, "detail,19,%s,%s\n", name.c_str(), ok ? "PASS" : "FAIL");
        if (!ok)
        {
            ++failCount;
        }
    };

    LockFreeMap<int32_t, int32_t> defaultMap;
    check(defaultMap.empty() && defaultMap.count() == 0 && defaultMap.size() == 0, "LockFreeMap empty default");
    check(defaultMap.maxLevel() == 16, "LockFreeMap default max level");
    check(defaultMap.forEach([](const int32_t&, const int32_t&) -> void {}) == 0, "LockFreeMap empty forEach");
    check(defaultMap.keys().empty() && defaultMap.values().empty() && defaultMap.toVector().empty(), "LockFreeMap empty snapshots");
    defaultMap.clear();
    defaultMap.clearAndFreeMemory();
    check(defaultMap.empty() && defaultMap.retainedNodeCount() == 0 && defaultMap.retainedValueCount() == 0, "LockFreeMap clear empty");
    defaultMap[42] = 420;
    int32_t defaultIndexValue = 0;
    check(defaultMap.get(42, defaultIndexValue) && defaultIndexValue == 420, "LockFreeMap operator[] insert");
    defaultMap[42] = 421;
    check(static_cast<int32_t>(defaultMap[42]) == 421 && defaultMap.count(42) == 1, "LockFreeMap operator[] update and read");
    LockFreeMap<int32_t, int32_t> operatorCopyMap;
    operatorCopyMap[7] = defaultMap[42];
    check(operatorCopyMap.value(7, 0) == 421, "LockFreeMap operator[] proxy assign");

    LockFreeMap<int32_t, std::string> map;
    std::string missingText;
    check(!map.get(10, missingText), "LockFreeMap missing get false");
    check(map.value(10, "none") == "none" && !map.contains(10), "LockFreeMap missing value contains");
    check(!map.erase(10), "LockFreeMap erase missing");
    check(map.insert(3, "three") && map.insert(1, "one") && map.insert(2, "two"), "LockFreeMap unordered insert");
    check(!map.insert(2, "TWO"), "LockFreeMap duplicate insert");
    check(map.set(2, "TWO") && map.set(4, "four"), "LockFreeMap set existing and new");
    std::string text;
    check(map.get(2, text) && text == "TWO", "LockFreeMap get updated");
    std::vector<int32_t> keys = map.keys();
    check(keys.size() == 4 && keys[0] == 1 && keys[1] == 2 && keys[2] == 3 && keys[3] == 4, "LockFreeMap keys sorted");
    int32_t boundKey = 0;
    std::string boundValue;
    check(map.first(boundKey, boundValue) && boundKey == 1 && boundValue == "one", "LockFreeMap first");
    check(map.last(boundKey, boundValue) && boundKey == 4 && boundValue == "four", "LockFreeMap last");
    check(map.lowerBound(2, boundKey, boundValue) && boundKey == 2 && boundValue == "TWO", "LockFreeMap lowerBound equal");
    check(map.upperBound(2, boundKey, boundValue) && boundKey == 3, "LockFreeMap upperBound middle");
    check(!map.upperBound(4, boundKey, boundValue), "LockFreeMap upperBound after last");
    check(map.erase(2) && !map.erase(2), "LockFreeMap erase duplicate");
    check(!map.get(2, text) && map.count(2) == 0, "LockFreeMap erased key hidden");
    check(map.lowerBound(2, boundKey, boundValue) && boundKey == 3, "LockFreeMap lowerBound skips erased");
    check(map.insert(2, "again") && map.get(2, text) && text == "again", "LockFreeMap insert after erase");
    size_t retainedBeforeClear = map.retainedNodeCount();
    map.clear();
    check(map.empty() && !map.contains(1) && map.retainedNodeCount() >= retainedBeforeClear, "LockFreeMap logical clear");
    check(map.insert(1, "afterClear") && map.count() == 1, "LockFreeMap insert after clear");
    map.clearAndFreeMemory();
    check(map.empty() && map.retainedNodeCount() == 0 && map.retainedValueCount() == 0, "LockFreeMap clearAndFreeMemory");

    LockFreeMap<LockFreeMapHistoricalKey, LockFreeMapHistoricalValue, LockFreeMapHistoricalLess> throwMap;
    check(throwMap.set(LockFreeMapHistoricalKey(1), LockFreeMapHistoricalValue(10)), "LockFreeMap custom set");
    LockFreeMapHistoricalValue throwValue;
    size_t retainedNodeBeforeThrow = throwMap.retainedNodeCount();
    size_t retainedValueBeforeThrow = throwMap.retainedValueCount();
    LockFreeMapHistoricalFaults::m_less = true;
    check(!throwMap.set(LockFreeMapHistoricalKey(2), LockFreeMapHistoricalValue(20)), "LockFreeMap set less throw returns false");
    check(!throwMap.insert(LockFreeMapHistoricalKey(3), LockFreeMapHistoricalValue(30)), "LockFreeMap insert less throw returns false");
    check(!throwMap.erase(LockFreeMapHistoricalKey(1)), "LockFreeMap erase less throw returns false");
    check(!throwMap.contains(LockFreeMapHistoricalKey(1)) && !throwMap.get(LockFreeMapHistoricalKey(1), throwValue), "LockFreeMap read less throw returns false");
    LockFreeMapHistoricalFaults::m_less = false;
    LockFreeMapHistoricalFaults::m_keyCopy = true;
    check(!throwMap.set(LockFreeMapHistoricalKey(2), LockFreeMapHistoricalValue(20)), "LockFreeMap set key copy throw returns false");
    check(!throwMap.insert(LockFreeMapHistoricalKey(3), LockFreeMapHistoricalValue(30)), "LockFreeMap insert key copy throw returns false");
    LockFreeMapHistoricalFaults::m_keyCopy = false;
    LockFreeMapHistoricalFaults::m_valueCopy = true;
    check(!throwMap.set(LockFreeMapHistoricalKey(4), LockFreeMapHistoricalValue(40)), "LockFreeMap set value copy throw returns false");
    check(!throwMap.insert(LockFreeMapHistoricalKey(5), LockFreeMapHistoricalValue(50)), "LockFreeMap insert value copy throw returns false");
    LockFreeMapHistoricalFaults::m_valueCopy = false;
    LockFreeMapHistoricalFaults::m_valueAssign = true;
    check(!throwMap.get(LockFreeMapHistoricalKey(1), throwValue), "LockFreeMap get value assign throw returns false");
    LockFreeMapHistoricalFaults::m_valueAssign = false;
    check(throwMap.retainedNodeCount() == retainedNodeBeforeThrow && throwMap.retainedValueCount() == retainedValueBeforeThrow && throwMap.get(LockFreeMapHistoricalKey(1), throwValue) && throwValue.m_value == 10, "LockFreeMap throw branches keep old data");
    LockFreeMapHistoricalFaults::m_keyCopy = true;
    check(throwMap.keys().empty(), "LockFreeMap keys copy throw handled");
    LockFreeMapHistoricalFaults::m_keyCopy = false;
    LockFreeMapHistoricalFaults::m_valueCopy = true;
    check(throwMap.values().empty() && throwMap.toVector().empty(), "LockFreeMap values copy throw handled");
    LockFreeMapHistoricalFaults::m_valueCopy = false;
    size_t throwingCallbackCount = throwMap.forEach([](const LockFreeMapHistoricalKey&, const LockFreeMapHistoricalValue&) -> void
    {
        throw std::runtime_error("LockFreeMap callback failed");
    });
    check(throwingCallbackCount == 0, "LockFreeMap callback throw handled");
    check(throwMap.erase(LockFreeMapHistoricalKey(1)) && throwMap.empty(), "LockFreeMap custom erase");
    throwMap.clearAndFreeMemory();

    const int32_t threadCount = 8;
    // 固定负载使用静态常量，兼容 VS2013 对局部常量捕获的实现
    static const int32_t loopCount = 1000;
    LockFreeMap<int32_t, int32_t> concurrentMap;
    std::atomic<int32_t> failures(0);
    std::vector<LockFreeMapTestThread> threads;
    for (int32_t i = 0; i < threadCount; ++i)
    {
        threads.push_back(LockFreeMapTestThread([&concurrentMap, &failures, i]() -> void
        {
            int32_t base = i * loopCount;
            for (int32_t j = 0; j < loopCount; ++j)
            {
                int32_t key = base + j;
                if (!concurrentMap.set(key, key * 2))
                {
                    ++failures;
                    continue;
                }
                int32_t value = 0;
                if (!concurrentMap.get(key, value) || value != key * 2)
                {
                    ++failures;
                }
            }
        }));
    }
    for (size_t i = 0; i < threads.size(); ++i)
    {
        threads[i].join();
    }
    threads.clear();
    check(failures.load() == 0, "LockFreeMap concurrent set get failures");
    check(concurrentMap.count() == static_cast<size_t>(threadCount * loopCount), "LockFreeMap concurrent count");
    bool concurrentOrderOk = true;
    int32_t lastKey = -1;
    concurrentMap.forEach([&concurrentOrderOk, &lastKey](const int32_t& key, const int32_t&) -> void
    {
        if (key <= lastKey)
        {
            concurrentOrderOk = false;
        }
        lastKey = key;
    });
    check(concurrentOrderOk, "LockFreeMap concurrent order");
    for (int32_t i = 0; i < threadCount; ++i)
    {
        threads.push_back(LockFreeMapTestThread([&concurrentMap, &failures, i]() -> void
        {
            int32_t base = i * loopCount;
            for (int32_t j = 0; j < loopCount; j += 2)
            {
                int32_t key = base + j;
                if (!concurrentMap.erase(key))
                {
                    ++failures;
                }
            }
        }));
    }
    for (size_t i = 0; i < threads.size(); ++i)
    {
        threads[i].join();
    }
    threads.clear();
    check(failures.load() == 0, "LockFreeMap concurrent erase failures");
    check(concurrentMap.count() == static_cast<size_t>(threadCount * loopCount / 2), "LockFreeMap count after concurrent erase");

    LockFreeMap<int32_t, int32_t> sameKeyInsertMap;
    std::atomic<int32_t> sameKeyInsertSuccess(0);
    std::atomic<int32_t> sameKeyEraseSuccess(0);
    LockFreeMapTestThreadGroup sameKeyCleanup(threads);
    for (int32_t i = 0; i < threadCount * 4; ++i)
    {
        threads.push_back(LockFreeMapTestThread([&sameKeyInsertMap, &sameKeyInsertSuccess, i]() -> void
        {
            if (sameKeyInsertMap.insert(1, i))
            {
                ++sameKeyInsertSuccess;
            }
        }));
    }
    for (size_t i = 0; i < threads.size(); ++i)
    {
        threads[i].join();
    }
    threads.clear();
    check(sameKeyInsertSuccess.load() == 1 && sameKeyInsertMap.count() == 1, "LockFreeMap concurrent same key insert single success");
    for (int32_t i = 0; i < threadCount * 4; ++i)
    {
        threads.push_back(LockFreeMapTestThread([&sameKeyInsertMap, &sameKeyEraseSuccess]() -> void
        {
            if (sameKeyInsertMap.erase(1))
            {
                ++sameKeyEraseSuccess;
            }
        }));
    }
    for (size_t i = 0; i < threads.size(); ++i)
    {
        threads[i].join();
    }
    threads.clear();
    check(sameKeyEraseSuccess.load() == 1 && sameKeyInsertMap.empty(), "LockFreeMap concurrent same key erase single success");

    LockFreeMap<int32_t, int32_t> highStressMap;
    std::atomic<int32_t> stressFailures(0);
    std::atomic<int32_t> readyThreadCount(0);
    std::atomic<bool> startStress(false);
    const int32_t stressThreadCount = 16;
    static const int32_t stressLoopCount = 3000;
    LockFreeMapTestThreadGroup stressCleanup(threads, &startStress);
    for (int32_t i = 0; i < stressThreadCount; ++i)
    {
        threads.push_back(LockFreeMapTestThread([&highStressMap, &stressFailures, &readyThreadCount, &startStress, i]() -> void
        {
            ++readyThreadCount;
            const uint64_t deadline = LockFreeMapClock::milliseconds() + 10000;
            while (!startStress.load(std::memory_order_acquire) && LockFreeMapClock::milliseconds() < deadline)
            {
                std::this_thread::yield();
            }
            for (int32_t j = 0; j < stressLoopCount; ++j)
            {
                int32_t key = (i * 997 + j) % 512;
                if (!highStressMap.set(key, i * stressLoopCount + j))
                {
                    ++stressFailures;
                }
                int32_t value = 0;
                highStressMap.get(key, value);
                if (j % 3 == 0)
                {
                    highStressMap.insert(key + 10000, j);
                }
                if (j % 5 == 0)
                {
                    highStressMap.erase(key);
                }
                if (j % 127 == 0)
                {
                    highStressMap.forEach([](const int32_t&, const int32_t&) -> void {});
                    highStressMap.first(key, value);
                    highStressMap.last(key, value);
                }
                if (i == 0 && j % 1000 == 0)
                {
                    highStressMap.clear();
                }
            }
        }));
    }
    const uint64_t deadline = LockFreeMapClock::milliseconds() + 10000;
    while (readyThreadCount.load() != stressThreadCount && LockFreeMapClock::milliseconds() < deadline)
    {
        std::this_thread::yield();
    }
    stressFailures.fetch_add(readyThreadCount.load() == stressThreadCount ? 0 : 1);
    startStress.store(true, std::memory_order_release);
    for (size_t i = 0; i < threads.size(); ++i)
    {
        threads[i].join();
    }
    check(stressFailures.load() == 0, "LockFreeMap high stress mixed operations");
    highStressMap.clear();
    bool rebuildAfterStressOk = true;
    for (int32_t i = 0; i < 1024; ++i)
    {
        rebuildAfterStressOk = rebuildAfterStressOk && highStressMap.set(i, i * 3);
    }
    check(rebuildAfterStressOk && highStressMap.count() == 1024, "LockFreeMap rebuild after high stress");
    highStressMap.clearAndFreeMemory();

    return failCount == 0 ? 0 : 1;
}