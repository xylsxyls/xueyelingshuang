#include "LockFreeHashMapStdMutexTests.h"
#include "LockFreeHashMapTestThread.h"
#include <algorithm>
#include <iostream>
#include <map>
#include <memory>
#include <thread>

bool LockFreeHashMapStdMutexValue::s_throw = false;
uint64_t LockFreeHashMapStdMutexTests::s_checks = 0;
uint64_t LockFreeHashMapStdMutexTests::s_failed = 0;

LockFreeHashMapStdMutexValue::LockFreeHashMapStdMutexValue(int32_t value) :
m_value(value)
{

}

LockFreeHashMapStdMutexValue::LockFreeHashMapStdMutexValue(const LockFreeHashMapStdMutexValue& other) :
m_value(other.m_value)
{
    if (s_throw)
    {
        throw 31;
    }
}

LockFreeHashMapStdMutexValue& LockFreeHashMapStdMutexValue::operator=(const LockFreeHashMapStdMutexValue& other)
{
    if (s_throw)
    {
        throw 32;
    }
    m_value = other.m_value;
    return *this;
}

size_t LockFreeHashMapStdMutexHash::operator()(int32_t key) const
{
    (void)key;
    return 0;
}

void LockFreeHashMapStdMutexTests::check(bool condition, const char* name)
{
    ++s_checks;
    if (!condition)
    {
        ++s_failed;
        std::cerr << "STD_CHECK_FAILED " << name << std::endl;
        throw 91;
    }
}

void LockFreeHashMapStdMutexTests::model()
{
    LockFreeHashMapStdMutex<int32_t, int32_t, LockFreeHashMapStdMutexHash> actual(1);
    std::map<int32_t, int32_t> expected;
    uint32_t random = UINT32_C(0x5eed1234);
    for (int32_t iteration = 0; iteration < 20000; ++iteration)
    {
        random = random * 1664525U + 1013904223U;
        int32_t key = static_cast<int32_t>((random >> 8) % 64);
        switch (random % 5)
        {
        case 0:
        {
            check(actual.set(key, iteration), "model set succeeds");
            expected[key] = iteration;
            break;
        }
        case 1:
        {
            bool inserted = expected.insert(std::make_pair(key, iteration)).second;
            check(actual.insert(key, iteration) == inserted, "model insert is conditional");
            break;
        }
        case 2:
        {
            check(actual.erase(key) == (expected.erase(key) != 0), "model erase agrees");
            break;
        }
        default:
        {
            int32_t value = -1;
            std::map<int32_t, int32_t>::const_iterator found = expected.find(key);
            bool exists = found != expected.end();
            check(actual.get(key, value) == exists && (!exists || value == found->second), "model exact value");
            check(actual.contains(key) == exists, "model contains agrees");
            break;
        }
        }
        if (iteration % 127 == 0)
        {
            std::vector<std::pair<int32_t, int32_t>> pairs = actual.toVector();
            std::map<int32_t, int32_t> observed(pairs.begin(), pairs.end());
            check(observed == expected && pairs.size() == expected.size(), "pair export has exact unique entries");
            std::vector<int32_t> keys = actual.keys();
            std::vector<int32_t> values = actual.values();
            check(keys.size() == expected.size() && values.size() == expected.size() && actual.size() == expected.size(), "all export sizes agree");
            std::vector<int32_t> expectedKeys, expectedValues;
            for (std::map<int32_t, int32_t>::const_iterator item = expected.begin(); item != expected.end(); ++item)
            {
                expectedKeys.push_back(item->first);
                expectedValues.push_back(item->second);
            }
            std::sort(keys.begin(), keys.end());
            std::sort(values.begin(), values.end());
            std::sort(expectedValues.begin(), expectedValues.end());
            check(keys == expectedKeys && values == expectedValues, "key and value exports preserve exact contents");
        }
        if (iteration % 1024 == 0)
        {
            actual.clear();
            expected.clear();
            check(actual.size() == 0, "clear empties standard table");
        }
    }
    actual.clearAndFreeMemory();
    check(actual.size() == 0 && actual.set(7, 9), "explicit release permits reuse");
}

void LockFreeHashMapStdMutexTests::exceptionsAndOwnership()
{
    LockFreeHashMapStdMutex<int32_t, LockFreeHashMapStdMutexValue> actual;
    LockFreeHashMapStdMutexValue value(7), output;
    check(actual.set(1, value), "exception fixture initialized");
    LockFreeHashMapStdMutexValue::s_throw = true;
    bool read = actual.get(1, output);
    bool inserted = actual.insert(2, value);
    LockFreeHashMapStdMutexValue::s_throw = false;
    check(!read && !inserted && actual.size() == 1 && actual.get(1, output) && output.m_value == 7, "copy exceptions release lock and preserve existing item");
    // 在第一个回调抛出故障，后续 get 必须能重新取得互斥锁
    size_t visited = actual.forEach([](const int32_t& key, const LockFreeHashMapStdMutexValue& item)
    {
        (void)key;
        (void)item;
        throw 33;
    });
    check(visited == 0 && actual.get(1, output), "callback exception releases traversal lock");
    LockFreeHashMapStdMutex<int32_t, std::shared_ptr<int32_t>> owned;
    std::shared_ptr<int32_t> candidate = std::make_shared<int32_t>(19);
    std::weak_ptr<int32_t> weak = candidate;
    check(owned.set(1, candidate), "owned fixture initialized");
    std::shared_ptr<int32_t> copy;
    check(owned.get(1, copy), "get creates independent shared owner");
    candidate.reset();
    owned.clearAndFreeMemory();
    check(!weak.expired() && *copy == 19, "read owner survives clear and storage release");
    copy.reset();
    check(weak.expired(), "last external owner releases resource");
}

void LockFreeHashMapStdMutexTests::concurrent()
{
    LockFreeHashMapStdMutex<int32_t, int32_t, LockFreeHashMapStdMutexHash> actual(1);
    std::atomic<int32_t> winners(0), errors(0);
    std::atomic<bool> start(false);
    std::vector<LockFreeHashMapTestThread> threads;
    threads.reserve(8);
    try
    {
        for (int32_t thread = 0; thread < 8; ++thread)
        {
            // 工作者共用插入目标，随后只改各自键区；错误以原子计数交给主线程
            threads.push_back(LockFreeHashMapTestThread([&, thread]()
            {
                while (!start.load())
                {
                    std::this_thread::yield();
                }
                if (actual.insert(-1, 7))
                {
                    winners.fetch_add(1);
                }
                for (int32_t iteration = 0; iteration < 10000; ++iteration)
                {
                    int32_t key = thread * 16 + iteration % 16;
                    int32_t value = -1;
                    if (!actual.set(key, iteration) || !actual.get(key, value) || value != iteration)
                    {
                        errors.fetch_add(1);
                    }
                    if (iteration % 257 == 0)
                    {
                        // 在整次遍历锁内核对范围；不从回调重入同一标准表
                        actual.forEach([&](const int32_t& itemKey, const int32_t& itemValue)
                        {
                            if (itemKey < -1 || itemKey >= 128 || itemValue < 0 || itemValue >= 10000)
                            {
                                errors.fetch_add(1);
                            }
                        });
                    }
                }
            }));
        }
    }
    catch (...)
    {
        start.store(true);
        throw;
    }
    start.store(true);
    for (size_t i = 0; i < threads.size(); ++i)
    {
        threads[i].join();
    }
    check(winners.load() == 1 && errors.load() == 0 && actual.size() == 129, "same-key unique insert and eighty thousand concurrent updates");
}

int LockFreeHashMapStdMutexTests::run()
{
    try
    {
        model();
        exceptionsAndOwnership();
        int before = LockFreeHashMapTestThread::failures();
        concurrent();
        check(LockFreeHashMapTestThread::failures() == before, "no hidden worker exception");
    }
    catch (...)
    {
        LockFreeHashMapStdMutexValue::s_throw = false;
        ++s_failed;
    }
    std::cout << "MODULE_RESULT checks=" << s_checks << " failed=" << s_failed << std::endl;
    return s_failed == 0 ? 0 : 1;
}

/** 标准容器适配器的独立验证入口
@return 零为全部通过
*/
int main()
{
    return LockFreeHashMapStdMutexTests::run();
}