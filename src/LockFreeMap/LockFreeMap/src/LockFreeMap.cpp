#include "LockFreeMap.h"

//#include "LockFreeMap.h"
//#include <atomic>
//#include <functional>
//#include <iostream>
//#include <stdint.h>
//#include <stdexcept>
//#include <string>
//#include <thread>
//#include <vector>
//
//static bool g_lockFreeMapThrowKeyCopy = false;
//static bool g_lockFreeMapThrowValueCopy = false;
//static bool g_lockFreeMapThrowValueAssign = false;
//static bool g_lockFreeMapThrowLess = false;
//
//struct LockFreeMapTestKey
//{
//public:
//    int32_t m_value;
//
//public:
//    explicit LockFreeMapTestKey(int32_t value = 0) :
//    m_value(value)
//    {
//
//    }
//
//    LockFreeMapTestKey(const LockFreeMapTestKey& other) :
//    m_value(other.m_value)
//    {
//        if (g_lockFreeMapThrowKeyCopy)
//        {
//            throw std::runtime_error("LockFreeMapTestKey copy failed");
//        }
//    }
//
//    LockFreeMapTestKey& operator=(const LockFreeMapTestKey& other)
//    {
//        if (this == &other)
//        {
//            return *this;
//        }
//        if (g_lockFreeMapThrowKeyCopy)
//        {
//            throw std::runtime_error("LockFreeMapTestKey assign failed");
//        }
//        m_value = other.m_value;
//        return *this;
//    }
//};
//
//struct LockFreeMapTestValue
//{
//public:
//    int32_t m_value;
//
//public:
//    explicit LockFreeMapTestValue(int32_t value = 0) :
//    m_value(value)
//    {
//
//    }
//
//    LockFreeMapTestValue(const LockFreeMapTestValue& other) :
//    m_value(other.m_value)
//    {
//        if (g_lockFreeMapThrowValueCopy)
//        {
//            throw std::runtime_error("LockFreeMapTestValue copy failed");
//        }
//    }
//
//    LockFreeMapTestValue& operator=(const LockFreeMapTestValue& other)
//    {
//        if (this == &other)
//        {
//            return *this;
//        }
//        if (g_lockFreeMapThrowValueAssign)
//        {
//            throw std::runtime_error("LockFreeMapTestValue assign failed");
//        }
//        m_value = other.m_value;
//        return *this;
//    }
//};
//
//struct LockFreeMapTestLess
//{
//public:
//    bool operator()(const LockFreeMapTestKey& left, const LockFreeMapTestKey& right) const
//    {
//        if (g_lockFreeMapThrowLess)
//        {
//            throw std::runtime_error("LockFreeMapTestLess failed");
//        }
//        return left.m_value < right.m_value;
//    }
//};
//
//int main()
//{
//    int32_t totalCount = 0;
//    int32_t failCount = 0;
//    std::function<void(bool, const std::string&)> check = [&totalCount, &failCount](bool ok, const std::string& name) -> void
//    {
//        ++totalCount;
//        std::cout << (ok ? "[PASS] " : "[FAIL] ") << name << std::endl;
//        if (!ok)
//        {
//            ++failCount;
//        }
//    };
//
//    LockFreeMap<int32_t, int32_t> defaultMap;
//    check(defaultMap.empty() && defaultMap.count() == 0 && defaultMap.size() == 0, "LockFreeMap empty default");
//    check(defaultMap.maxLevel() == 16, "LockFreeMap default max level");
//    check(defaultMap.forEach([](const int32_t&, const int32_t&) -> void {}) == 0, "LockFreeMap empty forEach");
//    check(defaultMap.keys().empty() && defaultMap.values().empty() && defaultMap.toVector().empty(), "LockFreeMap empty snapshots");
//    defaultMap.clear();
//    defaultMap.clearAndFreeMemory();
//    check(defaultMap.empty() && defaultMap.retainedNodeCount() == 0 && defaultMap.retainedValueCount() == 0, "LockFreeMap clear empty");
//    defaultMap[42] = 420;
//    int32_t defaultIndexValue = 0;
//    check(defaultMap.get(42, defaultIndexValue) && defaultIndexValue == 420, "LockFreeMap operator[] insert");
//    defaultMap[42] = 421;
//    check(static_cast<int32_t>(defaultMap[42]) == 421 && defaultMap.count(42) == 1, "LockFreeMap operator[] update and read");
//    LockFreeMap<int32_t, int32_t> operatorCopyMap;
//    operatorCopyMap[7] = defaultMap[42];
//    check(operatorCopyMap.value(7, 0) == 421, "LockFreeMap operator[] proxy assign");
//
//    LockFreeMap<int32_t, std::string> map;
//    std::string missingText;
//    check(!map.get(10, missingText), "LockFreeMap missing get false");
//    check(map.value(10, "none") == "none" && !map.contains(10), "LockFreeMap missing value contains");
//    check(!map.erase(10), "LockFreeMap erase missing");
//    check(map.insert(3, "three") && map.insert(1, "one") && map.insert(2, "two"), "LockFreeMap unordered insert");
//    check(!map.insert(2, "TWO"), "LockFreeMap duplicate insert");
//    check(map.set(2, "TWO") && map.set(4, "four"), "LockFreeMap set existing and new");
//    std::string text;
//    check(map.get(2, text) && text == "TWO", "LockFreeMap get updated");
//    std::vector<int32_t> keys = map.keys();
//    check(keys.size() == 4 && keys[0] == 1 && keys[1] == 2 && keys[2] == 3 && keys[3] == 4, "LockFreeMap keys sorted");
//    int32_t boundKey = 0;
//    std::string boundValue;
//    check(map.first(boundKey, boundValue) && boundKey == 1 && boundValue == "one", "LockFreeMap first");
//    check(map.last(boundKey, boundValue) && boundKey == 4 && boundValue == "four", "LockFreeMap last");
//    check(map.lowerBound(2, boundKey, boundValue) && boundKey == 2 && boundValue == "TWO", "LockFreeMap lowerBound equal");
//    check(map.upperBound(2, boundKey, boundValue) && boundKey == 3, "LockFreeMap upperBound middle");
//    check(!map.upperBound(4, boundKey, boundValue), "LockFreeMap upperBound after last");
//    check(map.erase(2) && !map.erase(2), "LockFreeMap erase duplicate");
//    check(!map.get(2, text) && map.count(2) == 0, "LockFreeMap erased key hidden");
//    check(map.lowerBound(2, boundKey, boundValue) && boundKey == 3, "LockFreeMap lowerBound skips erased");
//    check(map.insert(2, "again") && map.get(2, text) && text == "again", "LockFreeMap insert after erase");
//    size_t retainedBeforeClear = map.retainedNodeCount();
//    map.clear();
//    check(map.empty() && !map.contains(1) && map.retainedNodeCount() >= retainedBeforeClear, "LockFreeMap logical clear");
//    check(map.insert(1, "afterClear") && map.count() == 1, "LockFreeMap insert after clear");
//    map.clearAndFreeMemory();
//    check(map.empty() && map.retainedNodeCount() == 0 && map.retainedValueCount() == 0, "LockFreeMap clearAndFreeMemory");
//
//    LockFreeMap<LockFreeMapTestKey, LockFreeMapTestValue, LockFreeMapTestLess> throwMap;
//    check(throwMap.set(LockFreeMapTestKey(1), LockFreeMapTestValue(10)), "LockFreeMap custom set");
//    LockFreeMapTestValue throwValue;
//    size_t retainedNodeBeforeThrow = throwMap.retainedNodeCount();
//    size_t retainedValueBeforeThrow = throwMap.retainedValueCount();
//    g_lockFreeMapThrowLess = true;
//    check(!throwMap.set(LockFreeMapTestKey(2), LockFreeMapTestValue(20)), "LockFreeMap set less throw returns false");
//    check(!throwMap.insert(LockFreeMapTestKey(3), LockFreeMapTestValue(30)), "LockFreeMap insert less throw returns false");
//    check(!throwMap.erase(LockFreeMapTestKey(1)), "LockFreeMap erase less throw returns false");
//    check(!throwMap.contains(LockFreeMapTestKey(1)) && !throwMap.get(LockFreeMapTestKey(1), throwValue), "LockFreeMap read less throw returns false");
//    g_lockFreeMapThrowLess = false;
//    g_lockFreeMapThrowKeyCopy = true;
//    check(!throwMap.set(LockFreeMapTestKey(2), LockFreeMapTestValue(20)), "LockFreeMap set key copy throw returns false");
//    check(!throwMap.insert(LockFreeMapTestKey(3), LockFreeMapTestValue(30)), "LockFreeMap insert key copy throw returns false");
//    g_lockFreeMapThrowKeyCopy = false;
//    g_lockFreeMapThrowValueCopy = true;
//    check(!throwMap.set(LockFreeMapTestKey(4), LockFreeMapTestValue(40)), "LockFreeMap set value copy throw returns false");
//    check(!throwMap.insert(LockFreeMapTestKey(5), LockFreeMapTestValue(50)), "LockFreeMap insert value copy throw returns false");
//    g_lockFreeMapThrowValueCopy = false;
//    g_lockFreeMapThrowValueAssign = true;
//    check(!throwMap.get(LockFreeMapTestKey(1), throwValue), "LockFreeMap get value assign throw returns false");
//    g_lockFreeMapThrowValueAssign = false;
//    check(throwMap.retainedNodeCount() == retainedNodeBeforeThrow && throwMap.retainedValueCount() == retainedValueBeforeThrow && throwMap.get(LockFreeMapTestKey(1), throwValue) && throwValue.m_value == 10, "LockFreeMap throw branches keep old data");
//    g_lockFreeMapThrowKeyCopy = true;
//    check(throwMap.keys().empty(), "LockFreeMap keys copy throw handled");
//    g_lockFreeMapThrowKeyCopy = false;
//    g_lockFreeMapThrowValueCopy = true;
//    check(throwMap.values().empty() && throwMap.toVector().empty(), "LockFreeMap values copy throw handled");
//    g_lockFreeMapThrowValueCopy = false;
//    size_t throwingCallbackCount = throwMap.forEach([](const LockFreeMapTestKey&, const LockFreeMapTestValue&) -> void
//    {
//        throw std::runtime_error("LockFreeMap callback failed");
//    });
//    check(throwingCallbackCount == 0, "LockFreeMap callback throw handled");
//    check(throwMap.erase(LockFreeMapTestKey(1)) && throwMap.empty(), "LockFreeMap custom erase");
//    throwMap.clearAndFreeMemory();
//
//    const int32_t threadCount = 8;
//    const int32_t loopCount = 1000;
//    LockFreeMap<int32_t, int32_t> concurrentMap;
//    std::atomic<int32_t> failures(0);
//    std::vector<std::thread> threads;
//    for (int32_t i = 0; i < threadCount; ++i)
//    {
//        threads.push_back(std::thread([&concurrentMap, &failures, i, loopCount]() -> void
//        {
//            int32_t base = i * loopCount;
//            for (int32_t j = 0; j < loopCount; ++j)
//            {
//                int32_t key = base + j;
//                if (!concurrentMap.set(key, key * 2))
//                {
//                    ++failures;
//                    continue;
//                }
//                int32_t value = 0;
//                if (!concurrentMap.get(key, value) || value != key * 2)
//                {
//                    ++failures;
//                }
//            }
//        }));
//    }
//    for (size_t i = 0; i < threads.size(); ++i)
//    {
//        threads[i].join();
//    }
//    threads.clear();
//    check(failures.load() == 0, "LockFreeMap concurrent set get failures");
//    check(concurrentMap.count() == static_cast<size_t>(threadCount * loopCount), "LockFreeMap concurrent count");
//    bool concurrentOrderOk = true;
//    int32_t lastKey = -1;
//    concurrentMap.forEach([&concurrentOrderOk, &lastKey](const int32_t& key, const int32_t&) -> void
//    {
//        if (key <= lastKey)
//        {
//            concurrentOrderOk = false;
//        }
//        lastKey = key;
//    });
//    check(concurrentOrderOk, "LockFreeMap concurrent order");
//    for (int32_t i = 0; i < threadCount; ++i)
//    {
//        threads.push_back(std::thread([&concurrentMap, &failures, i, loopCount]() -> void
//        {
//            int32_t base = i * loopCount;
//            for (int32_t j = 0; j < loopCount; j += 2)
//            {
//                int32_t key = base + j;
//                if (!concurrentMap.erase(key))
//                {
//                    ++failures;
//                }
//            }
//        }));
//    }
//    for (size_t i = 0; i < threads.size(); ++i)
//    {
//        threads[i].join();
//    }
//    threads.clear();
//    check(failures.load() == 0, "LockFreeMap concurrent erase failures");
//    check(concurrentMap.count() == static_cast<size_t>(threadCount * loopCount / 2), "LockFreeMap count after concurrent erase");
//
//    LockFreeMap<int32_t, int32_t> sameKeyInsertMap;
//    std::atomic<int32_t> sameKeyInsertSuccess(0);
//    std::atomic<int32_t> sameKeyEraseSuccess(0);
//    for (int32_t i = 0; i < threadCount * 4; ++i)
//    {
//        threads.push_back(std::thread([&sameKeyInsertMap, &sameKeyInsertSuccess, i]() -> void
//        {
//            if (sameKeyInsertMap.insert(1, i))
//            {
//                ++sameKeyInsertSuccess;
//            }
//        }));
//    }
//    for (size_t i = 0; i < threads.size(); ++i)
//    {
//        threads[i].join();
//    }
//    threads.clear();
//    check(sameKeyInsertSuccess.load() == 1 && sameKeyInsertMap.count() == 1, "LockFreeMap concurrent same key insert single success");
//    for (int32_t i = 0; i < threadCount * 4; ++i)
//    {
//        threads.push_back(std::thread([&sameKeyInsertMap, &sameKeyEraseSuccess]() -> void
//        {
//            if (sameKeyInsertMap.erase(1))
//            {
//                ++sameKeyEraseSuccess;
//            }
//        }));
//    }
//    for (size_t i = 0; i < threads.size(); ++i)
//    {
//        threads[i].join();
//    }
//    threads.clear();
//    check(sameKeyEraseSuccess.load() == 1 && sameKeyInsertMap.empty(), "LockFreeMap concurrent same key erase single success");
//
//    LockFreeMap<int32_t, int32_t> highStressMap;
//    std::atomic<int32_t> stressFailures(0);
//    std::atomic<int32_t> readyThreadCount(0);
//    std::atomic<bool> startStress(false);
//    const int32_t stressThreadCount = 16;
//    const int32_t stressLoopCount = 3000;
//    for (int32_t i = 0; i < stressThreadCount; ++i)
//    {
//        threads.push_back(std::thread([&highStressMap, &stressFailures, &readyThreadCount, &startStress, i, stressLoopCount]() -> void
//        {
//            ++readyThreadCount;
//            while (!startStress.load(std::memory_order_acquire))
//            {
//                std::this_thread::yield();
//            }
//            for (int32_t j = 0; j < stressLoopCount; ++j)
//            {
//                int32_t key = (i * 997 + j) % 512;
//                if (!highStressMap.set(key, i * stressLoopCount + j))
//                {
//                    ++stressFailures;
//                }
//                int32_t value = 0;
//                highStressMap.get(key, value);
//                if (j % 3 == 0)
//                {
//                    highStressMap.insert(key + 10000, j);
//                }
//                if (j % 5 == 0)
//                {
//                    highStressMap.erase(key);
//                }
//                if (j % 127 == 0)
//                {
//                    highStressMap.forEach([](const int32_t&, const int32_t&) -> void {});
//                    highStressMap.first(key, value);
//                    highStressMap.last(key, value);
//                }
//                if (i == 0 && j % 1000 == 0)
//                {
//                    highStressMap.clear();
//                }
//            }
//        }));
//    }
//    while (readyThreadCount.load() != stressThreadCount)
//    {
//        std::this_thread::yield();
//    }
//    startStress.store(true, std::memory_order_release);
//    for (size_t i = 0; i < threads.size(); ++i)
//    {
//        threads[i].join();
//    }
//    check(stressFailures.load() == 0, "LockFreeMap high stress mixed operations");
//    highStressMap.clear();
//    bool rebuildAfterStressOk = true;
//    for (int32_t i = 0; i < 1024; ++i)
//    {
//        rebuildAfterStressOk = rebuildAfterStressOk && highStressMap.set(i, i * 3);
//    }
//    check(rebuildAfterStressOk && highStressMap.count() == 1024, "LockFreeMap rebuild after high stress");
//    highStressMap.clearAndFreeMemory();
//
//    std::cout << "LockFreeMap test " << (failCount == 0 ? "PASS" : "FAIL") << ", total=" << totalCount << ", failed=" << failCount << std::endl;
//    return failCount == 0 ? 0 : 1;
//}