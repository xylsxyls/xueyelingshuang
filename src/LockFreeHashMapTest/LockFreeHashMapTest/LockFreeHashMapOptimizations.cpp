#include "LockFreeHashMapTests.h"
#include "LockFreeHashMapTestState.h"
#include <array>
#include <cstring>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>

std::atomic<bool> LockFreeHashMapOnceEqual::s_throw(false);
std::atomic<int> LockFreeHashMapOnceEqual::s_calls(0);
std::atomic<size_t> LockFreeHashMapStringTestHash::s_calls(0);
std::atomic<bool> LockFreeHashMapStringTestEqual::s_throw(false);

size_t LockFreeHashMapStringTestHash::operator()(const std::string& key) const
{
    (void)key;
    s_calls.fetch_add(1);
    return 0;
}

bool LockFreeHashMapStringTestEqual::operator()(const std::string& left, const std::string& right) const
{
    if (s_throw.load())
    {
        throw 72;
    }
    return left == right;
}

bool LockFreeHashMapOnceEqual::operator()(int left, int right) const
{
    s_calls.fetch_add(1);
    if (s_throw.exchange(false))
    {
        throw std::runtime_error("one equality failure");
    }
    return left == right;
}

void LockFreeHashMapTestAccess::sequence(LockFreeHashMap<int, int>& map, uint64_t value)
{
    LockFreeHashMapTestState::sequence(map).store(value);
    LockFreeHashMap<int, int>::Storage& storage = LockFreeHashMapTestState::storage(map);
    for (size_t i = 0; i < storage.epoch()->count(); ++i)
    {
        LockFreeHashMapEpochRecord* record = storage.epoch()->record(i);
        record->m_versionNext = 0;
        record->m_versionEnd = 0;
    }
}

void LockFreeHashMapTestAccess::historySequence(LockFreeHashMap<int, int, LockFreeHashMapCollisionHash>& map, uint64_t value)
{
    LockFreeHashMapTestState::historySequence(map).store(value);
}

void LockFreeHashMapTestAccess::countOptimizations()
{
    LockFreeHashMap<int, int> map;
    LockFreeHashMapTests::check(map.set(1, 10), "sharded count fixture");
    LockFreeHashMapEpochRecord& record = *LockFreeHashMapTestState::storage(map).epoch()->record(0);
    record.m_countGeneration.store(LockFreeHashMapTestState::generation(map).load());
    record.m_countDelta.store((std::numeric_limits<int64_t>::max)());
    LockFreeHashMapTestState::account(map, record, LockFreeHashMapTestState::generation(map).load(), 1);
    LockFreeHashMapTests::check(!LockFreeHashMapTestState::countValid(map).load() && map.count() == 1,
        "sharded count overflow falls back to protected traversal");
    map.clear();
    LockFreeHashMapTestState::account(map, record, 1, -1);
    LockFreeHashMapTests::check(map.count() == 0 && map.set(2, 20) && map.count() == 1,
        "late old-generation accounting cannot restore cleared keys");
    map.clearAndFreeMemory();
    LockFreeHashMapPoolConfig config;
    LockFreeHashMapTests::check(map.configurePool(config).m_status == LockFreeHashMapOk &&
        LockFreeHashMapTestState::countValid(map).load() && map.count() == 0,
        "quiescent pool reconfiguration restores compact accounting");
}

void LockFreeHashMapTestAccess::optimizations()
{
    countOptimizations();
    readOptimizations();
    LockFreeHashMap<bool, bool> flags;
    LockFreeHashMap<unsigned char, unsigned short> subword;
    bool flag = false;
    unsigned short shortValue = 0;
    LockFreeHashMapTests::check(flags.set(true, false) && flags.get(true, flag) && !flag &&
        subword.set(0xa5, 0xc3d2) && subword.get(0xa5, shortValue) && shortValue == 0xc3d2,
        "boolean and subword copies preserve complete values");
    LockFreeHashMap<int, int, LockFreeHashMapCollisionHash, LockFreeHashMapModuloEqual> equivalent(1);
    LockFreeHashMapTests::check(equivalent.set(1, 10) && equivalent.set(11, 20) &&
        equivalent.size() == 1 && equivalent.value(21) == 20,
        "custom equivalent keys remain one entry without scalar cache bypass");
    LockFreeHashMap<int, int, LockFreeHashMapCollisionHash, LockFreeHashMapOnceEqual> throwing(1);
    bool prepared = throwing.set(1, 10) && throwing.set(2, 20);
    LockFreeHashMapTests::check(prepared, "one-shot equality fixture contains both colliding keys");
    LockFreeHashMapOnceEqual::s_calls.store(0);
    LockFreeHashMapOnceEqual::s_throw.store(true);
    LockFreeHashMapResult failed = throwing.trySet(1, 99);
    bool stopped = LockFreeHashMapOnceEqual::s_calls.load() == 1;
    LockFreeHashMapTests::check(stopped && failed.m_status == LockFreeHashMapUserException && !failed.m_committed &&
        throwing.value(1) == 10, "one-shot search exception is propagated without hidden retry");
#ifndef LOCKFREEHASHMAP_TEST_SINGLE_THREAD
    sharedContention();
#endif
}

std::atomic<int> LockFreeHashMapReadPayload::s_live(0);
std::atomic<int> LockFreeHashMapReadPayload::s_copies(0);
std::atomic<bool> LockFreeHashMapReadPayload::s_throwCopy(false);
std::atomic<int> LockFreeHashMapReadPayload::s_assignments(0);
std::atomic<bool> LockFreeHashMapReadPayload::s_pause(false);
std::atomic<bool> LockFreeHashMapReadPayload::s_throw(false);
std::atomic<bool> LockFreeHashMapReadPayload::s_entered(false);
std::atomic<bool> LockFreeHashMapReadPayload::s_resume(false);

LockFreeHashMapReadPayload::LockFreeHashMapReadPayload(int value) :
m_value(value)
{
    s_live.fetch_add(1);
}

LockFreeHashMapReadPayload::LockFreeHashMapReadPayload(const LockFreeHashMapReadPayload& other) :
m_value(other.m_value)
{
    s_copies.fetch_add(1);
    if (s_throwCopy.load())
    {
        throw std::runtime_error("injected read payload copy");
    }
    s_live.fetch_add(1);
}

LockFreeHashMapReadPayload& LockFreeHashMapReadPayload::operator=(const LockFreeHashMapReadPayload& other)
{
    s_assignments.fetch_add(1);
    if (s_throw.exchange(false))
    {
        throw std::runtime_error("one output assignment failure");
    }
    if (s_pause.exchange(false))
    {
        s_entered.store(true);
        if (!LockFreeHashMapTests::wait(s_resume, 10000))
        {
            throw std::runtime_error("paused output assignment timeout");
        }
    }
    m_value = other.m_value;
    return *this;
}

LockFreeHashMapReadPayload::~LockFreeHashMapReadPayload()
{
    s_live.fetch_sub(1);
}

LockFreeHashMapMoveCallback::LockFreeHashMapMoveCallback(int& calls, bool throwOnMove) :
m_calls(&calls), m_throwOnMove(throwOnMove)
{

}

LockFreeHashMapMoveCallback::LockFreeHashMapMoveCallback(LockFreeHashMapMoveCallback&& other) :
m_calls(other.m_calls), m_throwOnMove(other.m_throwOnMove)
{
    if (m_throwOnMove)
    {
        throw std::runtime_error("injected callback move");
    }
    other.m_calls = nullptr;
}

void LockFreeHashMapMoveCallback::operator()(const int& key, const LockFreeHashMapReadPayload& value) const
{
    (void)key;
    (void)value;
    if (m_calls != nullptr)
    {
        ++*m_calls;
    }
}

bool LockFreeHashMapModuloEqual::operator()(int left, int right) const
{
    return left % 10 == right % 10;
}

void LockFreeHashMapTestAccess::ignoreReadPayload(const int& key, const LockFreeHashMapReadPayload& value)
{
    (void)key;
    (void)value;
}

void LockFreeHashMapTestAccess::readOptimizations()
{
    {
        // 即使整数副本已存在，每次查询仍须执行用户哈希并报告其失败，不能静默绕过用户代码。
        LockFreeHashMap<int, int, LockFreeHashMapTestHash> cached;
        int output = -7;
        LockFreeHashMapTests::check(cached.set(7, 42) && cached.get(7, output) && output == 42,
            "custom hash cached read fixture");
        output = -7;
        bool found = false, present = false;
        LockFreeHashMapTestHash::s_fail.store(true);
        try
        {
            found = cached.get(7, output);
            present = cached.contains(7);
        }
        catch (...)
        {
            LockFreeHashMapTestHash::s_fail.store(false);
            throw;
        }
        LockFreeHashMapTestHash::s_fail.store(false);
        LockFreeHashMapTests::check(!found && !present && output == -7 &&
            cached.get(7, output) && output == 42,
            "cached get and contains retain the custom hash exception boundary");
    }
    typedef LockFreeHashMap<int, LockFreeHashMapReadPayload> Map;
    {
        // 独立字符串模型覆盖每个八字节分块及尾部；相同长度逐字节扰动，包含零和高位字节。
        std::map<std::string, int> reference;
        int marker = 1;
        for (size_t length = 0; length <= 80; ++length)
        {
            std::string key(length, '\0');
            for (size_t index = 0; index < length; ++index)
            {
                key[index] = static_cast<char>((length * 13 + index * 29) & 255);
            }
            reference.insert(std::make_pair(key, marker++));
            for (size_t index = 0; index < length; ++index)
            {
                std::string changed(key);
                changed[index] = static_cast<char>(static_cast<unsigned char>(changed[index]) ^ 128);
                reference.insert(std::make_pair(changed, marker++));
            }
        }
        LockFreeHashMapTests::check(reference.size() == 3321, "binary string generator has no unintended duplicate keys");
        LockFreeHashMap<std::string, int> strings;
        for (std::map<std::string, int>::const_iterator item = reference.begin(); item != reference.end(); ++item)
        {
            LockFreeHashMapTests::check(strings.set(item->first, item->second), "binary string model insert");
        }
        for (std::map<std::string, int>::const_iterator item = reference.begin(); item != reference.end(); ++item)
        {
            int output = -1;
            LockFreeHashMapTests::check(strings.get(item->first, output) && output == item->second,
                "binary string model preserves each complete key");
        }
        LockFreeHashMapTests::check(strings.size() == reference.size(), "binary string model exact size");
        LockFreeHashMap<std::string, int>::Snapshot original, taken;
        LockFreeHashMapTests::check(strings.find(std::string(), original).m_status == LockFreeHashMapOk &&
            original.m_value && *original.m_value == 1 &&
            strings.compareExchange(std::string(), original.m_version, 99).m_committed &&
            strings.eraseIf(std::string(), original.m_version).m_status == LockFreeHashMapVersionMismatch &&
            strings.take(std::string(), taken).m_committed && taken.m_value && *taken.m_value == 99,
            "empty string key version replacement and owned take");
        strings.clear();
        int unchanged = -7;
        LockFreeHashMapTests::check(strings.empty() && !strings.get(reference.rbegin()->first, unchanged) && unchanged == -7,
            "binary string clear preserves missed output");
        LockFreeHashMap<std::string, int, LockFreeHashMapStringTestHash> collisions(1);
        LockFreeHashMapStringTestHash::s_calls.store(0);
        for (size_t length = 0; length <= 16; ++length)
        {
            std::string key(length, '\0');
            LockFreeHashMapTests::check(collisions.set(key, static_cast<int>(length)), "custom string collision insertion");
        }
        LockFreeHashMapTests::check(collisions.size() == 17, "custom string collisions retain every key");
        for (size_t length = 0; length <= 16; ++length)
        {
            std::string key(length, '\0');
            int output = -1;
            size_t before = LockFreeHashMapStringTestHash::s_calls.load();
            LockFreeHashMapTests::check(collisions.get(key, output) && output == static_cast<int>(length) &&
                LockFreeHashMapStringTestHash::s_calls.load() == before + 1,
                "custom string hash is invoked exactly once per read");
        }
        LockFreeHashMap<std::string, int, std::hash<std::string>, LockFreeHashMapStringTestEqual> customEqual;
        const std::string key("a\0b", 3);
        LockFreeHashMapTests::check(customEqual.set(key, 17), "custom string equality fixture");
        LockFreeHashMapStringTestEqual::s_throw.store(true);
        bool found = false;
        try
        {
            found = customEqual.get(key, unchanged);
        }
        catch (...)
        {
            LockFreeHashMapStringTestEqual::s_throw.store(false);
            throw;
        }
        LockFreeHashMapStringTestEqual::s_throw.store(false);
        LockFreeHashMapTests::check(!found && unchanged == -7 && customEqual.get(key, unchanged) && unchanged == 17,
            "custom string equality failure preserves output and later recovers");
    }
    {
        LockFreeHashMap<std::string, std::shared_ptr<int>> strings;
        const std::string embedded("a\0b", 3);
        const std::string large(4096, 'x');
        std::shared_ptr<int> expected(new int(37)), output;
        LockFreeHashMapTests::check(strings.set(std::string(), expected) && strings.set(embedded, expected) &&
            strings.set(large, std::shared_ptr<int>()), "standard string hash read fixtures");
        LockFreeHashMapTests::check(strings.get(std::string(), output) && output == expected &&
            strings.get(embedded, output) && output == expected && strings.contains(large) &&
            strings.get(large, output) && !output && !strings.contains("a"),
            "standard string hash preserves empty, embedded-null and long-key identities");
        output = expected;
        strings.clear();
        LockFreeHashMapTests::check(!strings.get(embedded, output) && output == expected && !strings.contains(large),
            "standard string read observes clear and preserves missed output");
    }
    {
        typedef std::array<uint64_t, 64> Payload;
        LockFreeHashMapPoolConfig config;
        config.protectionRecordCount = 1;
        config.lookupCacheCapacity = 0;
        LockFreeHashMap<int, Payload> trivial(8, config);
        Payload input, output;
        for (size_t i = 0; i < input.size(); ++i)
        {
            input[i] = UINT64_C(0x1020304050607080) + i;
        }
        output.fill(17);
        const Payload untouched = output;
        LockFreeHashMapTests::check(trivial.set(1, input), "trivial assignment read fixture");
        bool refused = false;
        // 遍历占住唯一保护记录；只复制字节的读取也必须真实取得保护，不能访问未保护节点。
        LockFreeHashMapResult walked = trivial.forEachResult([&](const int&, const Payload&)
        {
            refused = !trivial.get(1, output) && output == untouched;
        });
        LockFreeHashMapTests::check(walked.m_status == LockFreeHashMapOk && refused,
            "trivial read refuses exhausted protection and preserves every output byte");
        LockFreeHashMapTests::check(trivial.get(1, output) && output == input &&
            !trivial.get(2, output) && output == input,
            "trivial read copies the complete payload and miss preserves it");
        trivial.clear();
        trivial.maintain(64);
        LockFreeHashMapTests::check(output == input && !trivial.contains(1),
            "trivial output owns its bytes after clear and reclamation");
    }
    {
        LockFreeHashMapPoolConfig config;
        config.initialCapacity = 8;
        config.protectionRecordCount = 1;
        config.lookupCacheCapacity = 0;
        LockFreeHashMap<int, std::shared_ptr<int>> shared(8, config);
        std::shared_ptr<int> published(new int(42));
        bool deleted = false, reentered = false;
        LockFreeHashMapTests::check(shared.set(1, published), "shared read reentry fixture");
        // 旧输出的删除器借用本地状态；get 完成旧值释放后它们仍然存活。
        std::shared_ptr<int> output(new int(9), [&](int* old)
        {
            delete old;
            deleted = true;
            reentered = shared.set(2, published) && shared.erase(1);
        });
        LockFreeHashMapTests::check(!shared.get(99, output) && !deleted && *output == 9,
            "missing shared read leaves old ownership untouched");
        LockFreeHashMapTests::check(shared.get(1, output) && output == published && deleted && reentered &&
            !shared.contains(1) && shared.value(2) == published,
            "old shared output releases outside protection and can reenter with one record");
        LockFreeHashMapTests::check(shared.set(3, std::shared_ptr<int>()) && shared.get(3, output) && !output,
            "present null shared value is a successful read");
        LockFreeHashMap<int, std::shared_ptr<int>, std::hash<int>, LockFreeHashMapTestEqual> custom(8, config);
        LockFreeHashMapTests::check(custom.set(1, published), "custom shared comparator fixture");
        output = published;
        bool found = false;
        LockFreeHashMapTestEqual::s_fail.store(true);
        try
        {
            found = custom.get(1, output);
        }
        catch (...)
        {
            LockFreeHashMapTestEqual::s_fail.store(false);
            throw;
        }
        LockFreeHashMapTestEqual::s_fail.store(false);
        LockFreeHashMapTests::check(!found && output == published && custom.contains(1),
            "shared value with custom comparator keeps the generic exception boundary");
    }
    {
        Map map(1);
        LockFreeHashMapPoolConfig config;
        config.lookupCacheCapacity = 0;
        LockFreeHashMapTests::check(map.configurePool(config).m_status == LockFreeHashMapOk &&
            map.set(1, LockFreeHashMapReadPayload(42)), "uncached read fixture ready");
        LockFreeHashMapReadPayload output;
        int copies = LockFreeHashMapReadPayload::s_copies.load();
        int assignments = LockFreeHashMapReadPayload::s_assignments.load();
        LockFreeHashMapReadPayload::s_throwCopy.store(true);
        bool read = map.get(1, output);
        LockFreeHashMapReadPayload::s_throwCopy.store(false);
        LockFreeHashMapTests::check(read && output.m_value == 42 &&
            LockFreeHashMapReadPayload::s_copies.load() == copies &&
            LockFreeHashMapReadPayload::s_assignments.load() == assignments + 1,
            "uncached get assigns once without constructing a temporary value");
        LockFreeHashMapReadPayload::s_throw.store(true);
        LockFreeHashMapTests::check(!map.get(1, output) &&
            LockFreeHashMapReadPayload::s_assignments.load() == assignments + 2 && map.contains(1),
            "uncached get reports assignment failure once and preserves the entry");
        // key、value 为调用期借用，捕获对象只用于注入回调复制失败，不执行输出修改
        auto callback = [output](const int& key, const LockFreeHashMapReadPayload& value)
        {
            (void)key;
            (void)value;
            (void)output;
        };
        LockFreeHashMapReadPayload::s_throwCopy.store(true);
        LockFreeHashMapResult walked = map.forEachResult(callback);
        size_t completed = map.forEach(callback);
        LockFreeHashMapReadPayload::s_throwCopy.store(false);
        LockFreeHashMapTests::check(walked.m_status == LockFreeHashMapUserException &&
            walked.m_count == 0 && completed == 0 && map.contains(1),
            "traversal catches callback copy failure before any invocation");
        // 借用已有 std::function，避免旧标准库复制目标失败时丢失内部申请的内存。
        // 开启目标复制故障后仍应成功，证明引用包装没有偷偷复制目标。
        std::function<void(const int&, const LockFreeHashMapReadPayload&)> ownedCallback(callback);
        copies = LockFreeHashMapReadPayload::s_copies.load();
        LockFreeHashMapReadPayload::s_throwCopy.store(true);
        walked = map.forEachResult(std::ref(ownedCallback));
        completed = map.forEach(std::cref(ownedCallback));
        LockFreeHashMapReadPayload::s_throwCopy.store(false);
        LockFreeHashMapTests::check(walked.m_status == LockFreeHashMapOk && walked.m_count == 1 &&
            completed == 1 && LockFreeHashMapReadPayload::s_copies.load() == copies,
            "reference-wrapped std::function traverses without copying its throwing target");
        int calls = 0;
        LockFreeHashMapMoveCallback movable(calls, false);
        walked = map.forEachResult(std::move(movable));
        LockFreeHashMapMoveCallback legacy(calls, false);
        completed = map.forEach(std::move(legacy));
        LockFreeHashMapTests::check(walked.m_status == LockFreeHashMapOk && walked.m_count == 1 &&
            completed == 1 && calls == 2, "move-only callback works through status and legacy traversal");
        LockFreeHashMapMoveCallback throwing(calls, true);
        walked = map.forEachResult(std::move(throwing));
        LockFreeHashMapMoveCallback throwingLegacy(calls, true);
        completed = map.forEach(std::move(throwingLegacy));
        LockFreeHashMapTests::check(walked.m_status == LockFreeHashMapUserException && walked.m_count == 0 &&
            completed == 0 && calls == 2 && map.contains(1), "callback move failure is caught without invoking or modifying the entry");
        walked = map.forEachResult(ignoreReadPayload);
        LockFreeHashMapTests::check(walked.m_status == LockFreeHashMapOk && walked.m_count == 1 &&
            map.forEach(ignoreReadPayload) == 1 && map.forEachResult<decltype(callback)>(callback).m_count == 1,
            "function references and explicitly selected lvalue callback templates retain their calling contract");
    }
}

void LockFreeHashMapTestAccess::sharedContention()
{
    typedef LockFreeHashMap<int, std::shared_ptr<int>, LockFreeHashMapCollisionHash> Map;
    Map map(1);
    LockFreeHashMapPoolConfig config;
    config.initialCapacity = 512;
    config.maximumCapacity = 512;
    config.protectionRecordCount = 16;
    config.lookupCacheCapacity = 4;
    LockFreeHashMapTests::check(map.configurePool(config).m_status == LockFreeHashMapOk,
        "shared same-key batch contention configuration");
    std::vector<std::shared_ptr<int>> values;
    std::vector<std::weak_ptr<int>> weak;
    values.reserve(64);
    weak.reserve(64);
    for (int i = 0; i < 64; ++i)
    {
        values.push_back(std::make_shared<int>(i));
        weak.push_back(values.back());
    }
    for (int i = 0; i < 4; ++i)
    {
        LockFreeHashMapTests::check(map.set(i, values[i]), "shared contention fixture publishes each initial key");
    }
    std::atomic<int> errors(0);
    uint64_t statuses[4][16] = {};
    std::atomic<bool> start(false);
    std::atomic<bool> finished(false);
    std::vector<LockFreeHashMapTestThread> threads;
    threads.reserve(4);
    LockFreeHashMapTestThread maintenance;
    try
    {
        // 无参数工作函数借用本用例夹具，结果在连接线程后读取，无返回值
        maintenance = LockFreeHashMapTestThread([&]()
        {
            try
            {
                while (!finished.load())
                {
                    if (map.maintain(17).m_status != LockFreeHashMapOk)
                    {
                        errors.fetch_add(1);
                    }
                    LockFreeHashMapPoolStats stats = {};
                    if (map.poolStats(stats).m_status != LockFreeHashMapOk || stats.budget > 512 || stats.peakBudget > 512)
                    {
                        errors.fetch_add(1);
                    }
                    // key、value 只在回调内借用；回调执行本场景校验或异常注入，无返回值
                    LockFreeHashMapResult walked = map.forEachResult([&](const int& key, const std::shared_ptr<int>& value)
                    {
                        if (key < 0 || key >= 4 || !value || *value < 0 || *value >= 64 || *value % 4 != key)
                        {
                            errors.fetch_add(1);
                        }
                    });
                    if (walked.m_status != LockFreeHashMapOk)
                    {
                        errors.fetch_add(1);
                    }
                    LockFreeHashMapTests::yieldThread();
                }
            }
            catch (...)
            {
                errors.fetch_add(1);
            }
        });
        for (int t = 0; t < 4; ++t)
        {
            // 无参数工作函数借用本用例夹具，结果在连接线程后读取，无返回值
            threads.push_back(LockFreeHashMapTestThread([&, t]()
            {
                try
                {
                    LockFreeHashMapTests::wait(start);
                    uint32_t random = UINT32_C(0x51a7c00d) + t;
                    for (int i = 0; i < 20000; ++i)
                    {
                        random = random * UINT32_C(1664525) + UINT32_C(1013904223);
                        int key = int((random >> 8) & 3);
                        const std::shared_ptr<int>& candidate = values[int((random >> 16) & 15) * 4 + key];
                        LockFreeHashMapResult result;
                        if (i % 257 == 0)
                        {
                            result = map.tryClear();
                            if (result.m_status != LockFreeHashMapOk || !result.m_committed)
                            {
                                errors.fetch_add(1);
                            }
                        }
                        else if (i % 13 == 0)
                        {
                            Map::Snapshot observed;
                            result = map.find(key, observed);
                            if (result.m_status == LockFreeHashMapOk)
                            {
                                if (!observed.m_value || !*observed.m_value || **observed.m_value % 4 != key)
                                {
                                    errors.fetch_add(1);
                                }
                                result = map.compareExchange(key, observed.m_version, candidate);
                                if (!result.m_committed && result.m_status != LockFreeHashMapNotFound &&
                                    result.m_status != LockFreeHashMapVersionMismatch && result.m_status != LockFreeHashMapCleared &&
                                    result.m_status != LockFreeHashMapCapacityLimit && result.m_status != LockFreeHashMapReservationBusy)
                                {
                                    errors.fetch_add(1);
                                }
                            }
                            else if (result.m_status != LockFreeHashMapNotFound && result.m_status != LockFreeHashMapCleared)
                            {
                                errors.fetch_add(1);
                            }
                        }
                        else if (i % 11 == 0)
                        {
                            Map::Snapshot actual;
                            result = map.insertIfAbsent(key, candidate, actual);
                            if (result.m_status == LockFreeHashMapOk || result.m_status == LockFreeHashMapExists)
                            {
                                if (!actual.m_value || !*actual.m_value || **actual.m_value % 4 != key ||
                                    result.m_committed != (result.m_status == LockFreeHashMapOk))
                                {
                                    errors.fetch_add(1);
                                }
                            }
                            else if ((result.m_status != LockFreeHashMapCleared && result.m_status != LockFreeHashMapCapacityLimit &&
                                result.m_status != LockFreeHashMapReservationBusy) || result.m_committed)
                            {
                                errors.fetch_add(1);
                            }
                        }
                        else if (i % 7 == 0)
                        {
                            result = map.tryErase(key);
                            if (result.m_status != LockFreeHashMapOk && result.m_status != LockFreeHashMapNotFound &&
                                result.m_status != LockFreeHashMapCleared && result.m_status != LockFreeHashMapReservationBusy)
                            {
                                errors.fetch_add(1);
                            }
                        }
                        else if (i % 3 == 0)
                        {
                            std::shared_ptr<int> got;
                            if (map.get(key, got) && (!got || *got < 0 || *got >= 64 || *got % 4 != key || got != values[*got]))
                            {
                                errors.fetch_add(1);
                            }
                        }
                        else
                        {
                            result = map.trySet(key, candidate);
                            if ((!result.m_committed || result.m_status != LockFreeHashMapOk) &&
                                (result.m_committed || (result.m_status != LockFreeHashMapCleared &&
                                result.m_status != LockFreeHashMapCapacityLimit && result.m_status != LockFreeHashMapReservationBusy)))
                            {
                                errors.fetch_add(1);
                            }
                        }
                        if (result.m_committed && result.m_status != LockFreeHashMapOk)
                        {
                            errors.fetch_add(1);
                        }
                        if (static_cast<unsigned>(result.m_status) < 16)
                        {
                            // 每个工作线程只写自己的统计行，避免诊断的共享原子争用掩盖交错
                            ++statuses[t][result.m_status];
                        }
                    }
                }
                catch (...)
                {
                    errors.fetch_add(1);
                }
            }));
        }
    }
    catch (...)
    {
        errors.fetch_add(1);
    }
    start.store(true);
    for (size_t i = 0; i < threads.size(); ++i)
    {
        threads[i].join();
    }
    finished.store(true);
    if (maintenance.joinable())
    {
        maintenance.join();
    }
    map.clear();
    for (size_t i = 0; i < 8; ++i)
    {
        map.maintain(64);
    }
    LockFreeHashMapPoolStats reclaimed = {};
    bool reclaimedAll = map.poolStats(reclaimed).m_status == LockFreeHashMapOk && reclaimed.occupied == 0 && reclaimed.retired == 0;
    map.clearAndFreeMemory();
    values.clear();
    bool expired = true;
    for (size_t i = 0; i < weak.size(); ++i)
    {
        expired = expired && weak[i].expired();
    }
    std::cout << "SHARED_STRESS workers=" << threads.size() << " operations=80000 errors=" << errors.load()
        << " all_expired=" << expired << std::endl;
    for (size_t i = 0; i < 16; ++i)
    {
        uint64_t count = 0;
        for (size_t t = 0; t < 4; ++t)
        {
            count += statuses[t][i];
        }
        std::cout << "SHARED_STATUS status=" << i << " count=" << count << std::endl;
    }
    LockFreeHashMapPoolStats finalStats = {};
    LockFreeHashMapTests::check(threads.size() == 4 && errors.load() == 0 && expired && reclaimedAll &&
        map.poolStats(finalStats).m_status == LockFreeHashMapOk && finalStats.capacity == 0,
        "80000 shared same-key operations with clear conditional writes and concurrent maintenance preserve ownership and budgets");
}