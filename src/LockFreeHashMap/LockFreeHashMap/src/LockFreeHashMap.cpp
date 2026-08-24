#include "LockFreeHashMap.h"

//#include "LockFreeHashMap.h"
//#include <atomic>
//#include <functional>
//#include <iostream>
//#include <stdint.h>
//#include <sstream>
//#include <stdexcept>
//#include <string>
//#include <thread>
//#include <vector>
//
//static bool g_lockFreeMapThrowKeyCopy = false;
//static bool g_lockFreeMapThrowValueCopy = false;
//static bool g_lockFreeMapThrowValueAssign = false;
//static bool g_lockFreeMapThrowHash = false;
//static bool g_lockFreeMapThrowEqual = false;
//
//struct LockFreeHashMapTestKey
//{
//    explicit LockFreeHashMapTestKey(int32_t value = 0) :
//    m_value(value)
//    {
//
//    }
//
//    LockFreeHashMapTestKey(const LockFreeHashMapTestKey& other) :
//    m_value(other.m_value)
//    {
//        if (g_lockFreeMapThrowKeyCopy)
//        {
//            throw std::runtime_error("LockFreeHashMapTestKey copy failed");
//        }
//    }
//
//    int32_t m_value;
//};
//
//struct LockFreeHashMapTestValue
//{
//    explicit LockFreeHashMapTestValue(int32_t value = 0) :
//    m_value(value)
//    {
//
//    }
//
//    LockFreeHashMapTestValue(const LockFreeHashMapTestValue& other) :
//    m_value(other.m_value)
//    {
//        if (g_lockFreeMapThrowValueCopy)
//        {
//            throw std::runtime_error("LockFreeHashMapTestValue copy failed");
//        }
//    }
//
//    LockFreeHashMapTestValue& operator=(const LockFreeHashMapTestValue& other)
//    {
//        if (this == &other)
//        {
//            return *this;
//        }
//        if (g_lockFreeMapThrowValueAssign)
//        {
//            throw std::runtime_error("LockFreeHashMapTestValue assign failed");
//        }
//        m_value = other.m_value;
//        return *this;
//    }
//
//    int32_t m_value;
//};
//
//struct LockFreeHashMapTestKeyHash
//{
//    size_t operator()(const LockFreeHashMapTestKey& key) const
//    {
//        if (g_lockFreeMapThrowHash)
//        {
//            throw std::runtime_error("LockFreeHashMapTestKey hash failed");
//        }
//        return static_cast<size_t>(key.m_value);
//    }
//};
//
//struct LockFreeHashMapTestKeyEqual
//{
//    bool operator()(const LockFreeHashMapTestKey& left, const LockFreeHashMapTestKey& right) const
//    {
//        if (g_lockFreeMapThrowEqual)
//        {
//            throw std::runtime_error("LockFreeHashMapTestKey equal failed");
//        }
//        return left.m_value == right.m_value;
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
//    LockFreeHashMap<int32_t, int32_t> defaultMap(0);
//    check(defaultMap.bucketCount() == 1024, "LockFreeHashMap zero bucket uses default");
//    check(defaultMap.empty() && defaultMap.forEach([](const int32_t&, const int32_t&) -> void {}) == 0, "LockFreeHashMap empty forEach");
//    check(defaultMap.keys().empty() && defaultMap.values().empty() && defaultMap.toVector().empty(), "LockFreeHashMap empty snapshots");
//    defaultMap.clear();
//    defaultMap.clear();
//    check(defaultMap.empty() && defaultMap.retainedNodeCount() == 0, "LockFreeHashMap repeated clear empty");
//    defaultMap.clearAndFreeMemory();
//    defaultMap.clearAndFreeMemory();
//    check(defaultMap.empty() && defaultMap.retainedNodeCount() == 0, "LockFreeHashMap repeated clearAndFreeMemory empty");
//    defaultMap[42] = 420;
//    int32_t defaultIndexValue = 0;
//    check(defaultMap.get(42, defaultIndexValue) && defaultIndexValue == 420, "LockFreeHashMap operator[] insert");
//    defaultMap[42] = 421;
//    check(static_cast<int32_t>(defaultMap[42]) == 421 && defaultMap.count(42) == 1, "LockFreeHashMap operator[] update and read");
//    LockFreeHashMap<int32_t, int32_t> operatorCopyMap(4);
//    operatorCopyMap[7] = defaultMap[42];
//    check(operatorCopyMap.value(7, 0) == 421, "LockFreeHashMap operator[] proxy assign");
//    defaultMap.clearAndFreeMemory();
//    operatorCopyMap.clearAndFreeMemory();
//
//    LockFreeHashMap<int32_t, int32_t> branchMap(7);
//    check(branchMap.bucketCount() == 8, "LockFreeHashMap bucket count rounds up");
//    int32_t missingValue = 0;
//    check(!branchMap.get(10, missingValue) && missingValue == 0 && branchMap.value(10, -1) == -1 && !branchMap.contains(10), "LockFreeHashMap missing key query");
//    size_t retainedBeforeFailedErase = branchMap.retainedNodeCount();
//    check(!branchMap.erase(10) && branchMap.retainedNodeCount() == retainedBeforeFailedErase, "LockFreeHashMap erase missing releases temp node");
//    check(branchMap.insert(10, 100), "LockFreeHashMap branch insert");
//    size_t retainedBeforeFailedInsert = branchMap.retainedNodeCount();
//    check(!branchMap.insert(10, 200) && branchMap.retainedNodeCount() == retainedBeforeFailedInsert, "LockFreeHashMap duplicate insert releases temp node");
//    check(branchMap.set(10, 300) && branchMap.set(10, 400) && branchMap.count() == 1, "LockFreeHashMap repeated set keeps one visible key");
//    int32_t intValue = 0;
//    check(branchMap.get(10, intValue) && intValue == 400, "LockFreeHashMap repeated set latest value");
//    size_t branchForEachReturn = branchMap.forEach([](const int32_t&, const int32_t&) -> void {});
//    check(branchForEachReturn == branchMap.count(), "LockFreeHashMap forEach return count");
//    check(branchMap.remove(10), "LockFreeHashMap remove alias existing");
//    size_t retainedBeforeDuplicateRemove = branchMap.retainedNodeCount();
//    check(!branchMap.remove(10) && branchMap.retainedNodeCount() == retainedBeforeDuplicateRemove, "LockFreeHashMap remove alias duplicate");
//    check(!branchMap.get(10, intValue) && branchMap.count(10) == 0 && branchMap.empty(), "LockFreeHashMap removed key hidden");
//    check(branchMap.insert(10, 500) && branchMap.count() == 1, "LockFreeHashMap insert after remove");
//    branchMap.clear();
//    check(branchMap.empty() && !branchMap.contains(10), "LockFreeHashMap clear hides branch data");
//    branchMap.clearAndFreeMemory();
//    check(branchMap.empty() && branchMap.retainedNodeCount() == 0, "LockFreeHashMap branch clearAndFreeMemory releases nodes");
//
//    LockFreeHashMap<LockFreeHashMapTestKey, LockFreeHashMapTestValue, LockFreeHashMapTestKeyHash, LockFreeHashMapTestKeyEqual> throwMap(4);
//    check(throwMap.set(LockFreeHashMapTestKey(1), LockFreeHashMapTestValue(10)), "LockFreeHashMap custom type set");
//    LockFreeHashMapTestValue throwValue;
//    size_t retainedBeforeThrow = throwMap.retainedNodeCount();
//    g_lockFreeMapThrowHash = true;
//    check(!throwMap.set(LockFreeHashMapTestKey(2), LockFreeHashMapTestValue(20)), "LockFreeHashMap set hash throw returns false");
//    check(!throwMap.insert(LockFreeHashMapTestKey(3), LockFreeHashMapTestValue(30)), "LockFreeHashMap insert hash throw returns false");
//    check(!throwMap.erase(LockFreeHashMapTestKey(1)), "LockFreeHashMap erase hash throw returns false");
//    check(!throwMap.contains(LockFreeHashMapTestKey(1)) && !throwMap.get(LockFreeHashMapTestKey(1), throwValue), "LockFreeHashMap read hash throw returns false");
//    g_lockFreeMapThrowHash = false;
//    g_lockFreeMapThrowKeyCopy = true;
//    check(!throwMap.set(LockFreeHashMapTestKey(2), LockFreeHashMapTestValue(20)), "LockFreeHashMap set key copy throw returns false");
//    check(!throwMap.insert(LockFreeHashMapTestKey(3), LockFreeHashMapTestValue(30)), "LockFreeHashMap insert key copy throw returns false");
//    check(!throwMap.erase(LockFreeHashMapTestKey(1)), "LockFreeHashMap erase key copy throw returns false");
//    g_lockFreeMapThrowKeyCopy = false;
//    g_lockFreeMapThrowValueCopy = true;
//    check(!throwMap.set(LockFreeHashMapTestKey(4), LockFreeHashMapTestValue(40)), "LockFreeHashMap set value copy throw returns false");
//    check(!throwMap.insert(LockFreeHashMapTestKey(5), LockFreeHashMapTestValue(50)), "LockFreeHashMap insert value copy throw returns false");
//    g_lockFreeMapThrowValueCopy = false;
//    g_lockFreeMapThrowEqual = true;
//    check(!throwMap.insert(LockFreeHashMapTestKey(1), LockFreeHashMapTestValue(20)), "LockFreeHashMap insert equal throw returns false");
//    check(!throwMap.erase(LockFreeHashMapTestKey(1)), "LockFreeHashMap erase equal throw returns false");
//    check(!throwMap.contains(LockFreeHashMapTestKey(1)) && !throwMap.get(LockFreeHashMapTestKey(1), throwValue), "LockFreeHashMap read equal throw returns false");
//    g_lockFreeMapThrowEqual = false;
//    g_lockFreeMapThrowValueAssign = true;
//    check(!throwMap.get(LockFreeHashMapTestKey(1), throwValue), "LockFreeHashMap get value assign throw returns false");
//    g_lockFreeMapThrowValueAssign = false;
//    check(throwMap.retainedNodeCount() == retainedBeforeThrow && throwMap.count() == 1 && throwMap.get(LockFreeHashMapTestKey(1), throwValue) && throwValue.m_value == 10, "LockFreeHashMap throw branches keep old data");
//    g_lockFreeMapThrowKeyCopy = true;
//    check(throwMap.keys().empty(), "LockFreeHashMap keys copy throw handled");
//    g_lockFreeMapThrowKeyCopy = false;
//    g_lockFreeMapThrowValueCopy = true;
//    check(throwMap.values().empty() && throwMap.toVector().empty(), "LockFreeHashMap values copy throw handled");
//    g_lockFreeMapThrowValueCopy = false;
//    size_t throwingCallbackCount = throwMap.forEach([](const LockFreeHashMapTestKey&, const LockFreeHashMapTestValue&) -> void
//    {
//        throw std::runtime_error("LockFreeHashMap callback failed");
//    });
//    check(throwingCallbackCount == 0, "LockFreeHashMap callback throw handled");
//    check(throwMap.erase(LockFreeHashMapTestKey(1)) && throwMap.empty(), "LockFreeHashMap custom type erase");
//    throwMap.clearAndFreeMemory();
//    check(throwMap.retainedNodeCount() == 0, "LockFreeHashMap custom type clearAndFreeMemory");
//
//    LockFreeHashMap<int32_t, std::string> map(8);
//    check(map.bucketCount() >= 8, "LockFreeHashMap bucket count normalized");
//    check(map.empty() && map.count() == 0 && map.size() == 0, "LockFreeHashMap empty default");
//    check(map.insert(1, "one"), "LockFreeHashMap insert first");
//    check(!map.insert(1, "uno"), "LockFreeHashMap insert duplicate");
//    check(map.count() == 1 && map.count(1) == 1 && map.contains(1), "LockFreeHashMap count contains");
//    std::string text;
//    check(map.get(1, text) && text == "one", "LockFreeHashMap get inserted");
//    check(map.set(1, "ONE"), "LockFreeHashMap set existing");
//    check(map.get(1, text) && text == "ONE", "LockFreeHashMap get updated");
//    check(map.set(2, "two") && map.set(3, "three"), "LockFreeHashMap set new keys");
//    check(map.count() == 3, "LockFreeHashMap size after set");
//    check(map.value(4, "none") == "none", "LockFreeHashMap value default");
//
//    int32_t forEachCount = 0;
//    map.forEach([&forEachCount](const int32_t&, const std::string&) -> void
//    {
//        ++forEachCount;
//    });
//    check(forEachCount == 3, "LockFreeHashMap forEach count");
//    check(map.keys().size() == 3, "LockFreeHashMap keys size");
//    check(map.values().size() == 3, "LockFreeHashMap values size");
//    check(map.toVector().size() == 3, "LockFreeHashMap toVector size");
//
//    check(map.erase(1), "LockFreeHashMap erase existing");
//    check(!map.erase(1), "LockFreeHashMap erase duplicate");
//    check(!map.get(1, text) && map.count(1) == 0, "LockFreeHashMap erased hidden old value");
//    check(map.insert(1, "again"), "LockFreeHashMap insert after erase");
//    check(map.get(1, text) && text == "again", "LockFreeHashMap get after erase insert");
//    size_t retainedBeforeClear = map.retainedNodeCount();
//    map.clear();
//    check(map.empty() && map.count() == 0 && !map.contains(1), "LockFreeHashMap logical clear");
//    check(map.retainedNodeCount() >= retainedBeforeClear, "LockFreeHashMap clear keeps retained nodes");
//    check(map.set(5, "five") && map.count() == 1, "LockFreeHashMap set after clear");
//    map.clearAndFreeMemory();
//    check(map.empty() && map.retainedNodeCount() == 0, "LockFreeHashMap clearAndFreeMemory");
//
//    const int32_t threadCount = 8;
//    const int32_t loopCount = 1000;
//    LockFreeHashMap<int32_t, int32_t> clearRaceMap(1024);
//    std::atomic<int32_t> clearRaceFailures(0);
//    std::vector<std::thread> clearRaceThreads;
//    clearRaceThreads.push_back(std::thread([&clearRaceMap]() -> void
//    {
//        for (int32_t i = 0; i < 200; ++i)
//        {
//            clearRaceMap.clear();
//        }
//    }));
//    for (int32_t i = 0; i < threadCount; ++i)
//    {
//        clearRaceThreads.push_back(std::thread([&clearRaceMap, &clearRaceFailures, i, loopCount]() -> void
//        {
//            int32_t base = i * loopCount;
//            for (int32_t j = 0; j < loopCount; ++j)
//            {
//                if (!clearRaceMap.set(base + j, j))
//                {
//                    ++clearRaceFailures;
//                }
//            }
//        }));
//    }
//    for (size_t i = 0; i < clearRaceThreads.size(); ++i)
//    {
//        clearRaceThreads[i].join();
//    }
//    check(clearRaceFailures.load() == 0, "LockFreeHashMap concurrent clear set failures");
//    clearRaceMap.clear();
//    bool clearRaceRebuildOk = true;
//    for (int32_t i = 0; i < 64; ++i)
//    {
//        clearRaceRebuildOk = clearRaceRebuildOk && clearRaceMap.set(i, i);
//    }
//    check(clearRaceRebuildOk && clearRaceMap.count() == 64, "LockFreeHashMap count after concurrent clear rebuild");
//
//    LockFreeHashMap<int32_t, int32_t> concurrentMap(32768);
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
//    check(failures.load() == 0, "LockFreeHashMap concurrent set get failures");
//    check(concurrentMap.count() == static_cast<size_t>(threadCount * loopCount), "LockFreeHashMap concurrent count");
//
//    LockFreeHashMap<int32_t, int32_t> operatorRaceMap(32768);
//    std::atomic<int32_t> operatorRaceFailures(0);
//    threads.clear();
//    for (int32_t i = 0; i < threadCount; ++i)
//    {
//        threads.push_back(std::thread([&operatorRaceMap, i, loopCount]() -> void
//        {
//            int32_t base = i * loopCount;
//            for (int32_t j = 0; j < loopCount; ++j)
//            {
//                int32_t key = base + j;
//                operatorRaceMap[key] = key + 1;
//            }
//        }));
//    }
//    for (size_t i = 0; i < threads.size(); ++i)
//    {
//        threads[i].join();
//    }
//    bool operatorRaceReadOk = operatorRaceMap.count() == static_cast<size_t>(threadCount * loopCount);
//    for (int32_t i = 0; i < threadCount && operatorRaceReadOk; ++i)
//    {
//        int32_t base = i * loopCount;
//        for (int32_t j = 0; j < loopCount; ++j)
//        {
//            int32_t key = base + j;
//            int32_t value = 0;
//            if (!operatorRaceMap.get(key, value) || value != key + 1)
//            {
//                operatorRaceReadOk = false;
//                ++operatorRaceFailures;
//                break;
//            }
//        }
//    }
//    check(operatorRaceFailures.load() == 0 && operatorRaceReadOk, "LockFreeHashMap concurrent operator[] set get");
//    operatorRaceMap.clearAndFreeMemory();
//    threads.clear();
//
//    std::atomic<int32_t> traverseCount(0);
//    concurrentMap.forEach([&traverseCount](const int32_t&, const int32_t&) -> void
//    {
//        ++traverseCount;
//    });
//    check(traverseCount.load() == threadCount * loopCount, "LockFreeHashMap concurrent forEach after writers");
//
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
//    check(failures.load() == 0, "LockFreeHashMap concurrent erase failures");
//    check(concurrentMap.count() == static_cast<size_t>(threadCount * loopCount / 2), "LockFreeHashMap count after concurrent erase");
//    bool oddKeysOk = true;
//    for (int32_t i = 0; i < threadCount && oddKeysOk; ++i)
//    {
//        int32_t base = i * loopCount;
//        for (int32_t j = 1; j < loopCount; j += 2)
//        {
//            int32_t value = 0;
//            int32_t key = base + j;
//            if (!concurrentMap.get(key, value) || value != key * 2)
//            {
//                oddKeysOk = false;
//                break;
//            }
//        }
//    }
//    check(oddKeysOk, "LockFreeHashMap odd keys remain after erase");
//
//    LockFreeHashMap<int32_t, int32_t> hotKeyMap(64);
//    threads.clear();
//    for (int32_t i = 0; i < threadCount; ++i)
//    {
//        threads.push_back(std::thread([&hotKeyMap, i, loopCount]() -> void
//        {
//            for (int32_t j = 0; j < loopCount; ++j)
//            {
//                hotKeyMap.set(j % 32, i * loopCount + j);
//            }
//        }));
//    }
//    for (size_t i = 0; i < threads.size(); ++i)
//    {
//        threads[i].join();
//    }
//    check(hotKeyMap.count() <= 32, "LockFreeHashMap hot key count not duplicated");
//    check(hotKeyMap.retainedNodeCount() >= hotKeyMap.count(), "LockFreeHashMap retained node count");
//    hotKeyMap.clearAndFreeMemory();
//    check(hotKeyMap.empty() && hotKeyMap.retainedNodeCount() == 0, "LockFreeHashMap hot key clearAndFreeMemory");
//
//    LockFreeHashMap<int32_t, int32_t> sameKeyInsertMap(8);
//    std::atomic<int32_t> sameKeyInsertSuccess(0);
//    std::atomic<int32_t> sameKeyEraseSuccess(0);
//    threads.clear();
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
//    check(sameKeyInsertSuccess.load() == 1 && sameKeyInsertMap.count() == 1, "LockFreeHashMap concurrent same key insert single success");
//    threads.clear();
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
//    check(sameKeyEraseSuccess.load() == 1 && sameKeyInsertMap.empty(), "LockFreeHashMap concurrent same key erase single success");
//
//    LockFreeHashMap<int32_t, int32_t> highStressMap(4096);
//    std::atomic<int32_t> stressFailures(0);
//    std::atomic<int32_t> readyThreadCount(0);
//    std::atomic<bool> startStress(false);
//    const int32_t stressThreadCount = 16;
//    const int32_t stressLoopCount = 3000;
//    threads.clear();
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
//                if (j % 7 == 0)
//                {
//                    highStressMap.contains(key);
//                }
//                if (j % 11 == 0)
//                {
//                    highStressMap.count(key);
//                }
//                if (j % 127 == 0)
//                {
//                    highStressMap.forEach([](const int32_t&, const int32_t&) -> void {});
//                }
//                if (j % 257 == 0)
//                {
//                    highStressMap.keys();
//                    highStressMap.values();
//                    highStressMap.toVector();
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
//    check(stressFailures.load() == 0, "LockFreeHashMap high stress mixed operations");
//    highStressMap.clear();
//    bool rebuildAfterStressOk = true;
//    for (int32_t i = 0; i < 1024; ++i)
//    {
//        rebuildAfterStressOk = rebuildAfterStressOk && highStressMap.set(i, i * 3);
//    }
//    bool rebuildReadBackOk = highStressMap.count() == 1024;
//    for (int32_t i = 0; i < 1024 && rebuildReadBackOk; ++i)
//    {
//        int32_t value = 0;
//        rebuildReadBackOk = highStressMap.get(i, value) && value == i * 3;
//    }
//    check(rebuildAfterStressOk && rebuildReadBackOk, "LockFreeHashMap rebuild after high stress");
//    highStressMap.clearAndFreeMemory();
//    check(highStressMap.empty() && highStressMap.retainedNodeCount() == 0, "LockFreeHashMap high stress clearAndFreeMemory");
//
//    std::cout << "LockFreeHashMap test " << (failCount == 0 ? "PASS" : "FAIL") << ", total=" << totalCount << ", failed=" << failCount << std::endl;
//    return failCount == 0 ? 0 : 1;
//}