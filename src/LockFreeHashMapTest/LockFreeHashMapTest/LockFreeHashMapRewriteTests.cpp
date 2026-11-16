#include "LockFreeHashMapRewriteTests.h"
#include "LockFreeHashMapTestState.h"
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <chrono>
#include <cstring>
#include <memory>
#include <new>
#include <type_traits>

std::atomic<bool> LockFreeHashMapRewriteHash::s_throw(false);
std::atomic<bool> LockFreeHashMapRewriteEqual::s_throw(false);
LOCKFREEHASHMAP_TEST_ALIGN_64 std::atomic<int64_t> LockFreeHashMapRewriteValue::s_live(0);
std::atomic<bool> LockFreeHashMapRewriteValue::s_throw(false);
std::atomic<int> LockFreeHashMapRewriteValue::s_pause(-1);
std::atomic<bool> LockFreeHashMapRewriteValue::s_entered(false);
std::atomic<bool> LockFreeHashMapRewriteValue::s_resume(false);
LOCKFREEHASHMAP_TEST_ALIGN_64 std::atomic<uint64_t> LockFreeHashMapRewriteTests::s_checks(0);
LOCKFREEHASHMAP_TEST_ALIGN_64 std::atomic<uint64_t> LockFreeHashMapRewriteTests::s_failed(0);

size_t LockFreeHashMapRewriteHash::operator()(const int& key) const
{
    (void)key;
    if (s_throw.load())
    {
        throw 31;
    }
    return 0;
}

bool LockFreeHashMapRewriteEqual::operator()(const int& left, const int& right) const
{
    if (s_throw.load())
    {
        throw 32;
    }
    return left == right;
}

LockFreeHashMapRewriteValue::LockFreeHashMapRewriteValue(int value) :
m_value(value)
{
    s_live.fetch_add(1);
}

LockFreeHashMapRewriteValue::LockFreeHashMapRewriteValue(const LockFreeHashMapRewriteValue& other) :
m_value(other.m_value)
{
    int expected = other.m_value;
    if (s_pause.compare_exchange_strong(expected, -1))
    {
        s_entered.store(true);
        while (!s_resume.load())
        {
            std::this_thread::yield();
        }
        // 暂停后再读取一次，提前释放或重用会改变独立预期
        m_value = other.m_value;
    }
    if (s_throw.load())
    {
        throw 33;
    }
    s_live.fetch_add(1);
}

LockFreeHashMapRewriteValue& LockFreeHashMapRewriteValue::operator=(const LockFreeHashMapRewriteValue& other)
{
    if (s_throw.load())
    {
        throw 34;
    }
    m_value = other.m_value;
    return *this;
}

LockFreeHashMapRewriteValue::~LockFreeHashMapRewriteValue()
{
    s_live.fetch_sub(1);
}

LockFreeHashMapAddressValue::LockFreeHashMapAddressValue(int value) :
m_value(value)
{

}

LockFreeHashMapAddressValue* LockFreeHashMapAddressValue::operator&()
{
    throw 92;
}

const LockFreeHashMapAddressValue* LockFreeHashMapAddressValue::operator&() const
{
    throw 93;
}

bool LockFreeHashMapAddressValue::operator==(const LockFreeHashMapAddressValue& other) const
{
    return m_value == other.m_value;
}

size_t LockFreeHashMapAddressHash::operator()(const LockFreeHashMapAddressValue& key) const
{
    return std::hash<int>()(key.m_value);
}

void LockFreeHashMapRewriteTests::check(bool condition, const char* name)
{
    s_checks.fetch_add(1);
    if (!condition)
    {
        s_failed.fetch_add(1);
        std::cerr << "REWRITE_FAILURE " << name << std::endl;
        throw 91;
    }
}

void LockFreeHashMapRewriteTests::addressableValues()
{
    typedef LockFreeHashMapAddressValue Value;
    typedef LockFreeHashMap<Value, Value, LockFreeHashMapAddressHash> Map;
    Map map(16);
    const Value key(7), first(11), second(13);
    Value output(-1);
    Map::Snapshot snapshot;
    try
    {
        check(map.set(key, first) && map.get(key, output) && output.m_value == 11,
            "overloaded address does not affect set or get");
        check(!map.insert(key, second) && map.contains(key), "overloaded address existing insert");
        check(map.insertIfAbsent(key, second, snapshot).m_status == LockFreeHashMapExists &&
            snapshot.m_value && snapshot.m_value->m_value == 11, "overloaded address conditional winner");
        check(map.compareExchange(key, snapshot.m_version, second).m_committed &&
            map.find(key, snapshot).m_status == LockFreeHashMapOk && snapshot.m_value->m_value == 13,
            "overloaded address compare exchange and snapshot");
        check(map.tryCompareExchange(key, snapshot.m_version, first, nullptr).m_committed,
            "overloaded address try compare exchange");
        check(map.trySet(key, second, &snapshot).m_committed && snapshot.m_value && snapshot.m_value->m_value == 13,
            "overloaded address try set");
        check(map.tryInsert(key, first).m_status == LockFreeHashMapExists &&
            map.tryInsertIfAbsent(key, first, snapshot).m_status == LockFreeHashMapExists,
            "overloaded address try insert interfaces");
        check(map.eraseIf(key, snapshot.m_version).m_committed && map.tryInsert(key, first).m_committed,
            "overloaded address conditional erase and new insert");
        check(map.take(key, snapshot).m_committed && snapshot.m_value && snapshot.m_value->m_value == 11 &&
            !map.contains(key), "overloaded address take owns the removed value");
        check(map.insert(key, second) && map.erase(key), "overloaded address normal insert and erase");
        map[key] = first;
        check(map.get(key, output) && output.m_value == 11 && map.values().size() == 1 && map.keys().size() == 1,
            "overloaded address proxy and collection exports");
        map.clear();
        check(!map.get(key, output) && output.m_value == 11, "overloaded address miss preserves output");
    }
    catch (int failure)
    {
        if (failure == 92 || failure == 93)
        {
            check(false, "container unexpectedly called overloaded operator address");
        }
        throw;
    }
    // 单独覆盖内建整数键的快速读取，不能只验证通用自定义键路径。
    LockFreeHashMap<int, Value> builtin;
    check(builtin.set(7, first) && builtin.get(7, output) && output.m_value == 11,
        "builtin key path also bypasses overloaded value address");
}

void LockFreeHashMapRewriteTests::sharedExports()
{
    typedef std::pair<int, int> Pair;
    typedef std::shared_ptr<int> Value;
    std::weak_ptr<Pair> lifetime;
    std::vector<Value> exported;
    {
        std::shared_ptr<Pair> owner = std::make_shared<Pair>(31, 47);
        lifetime = owner;
        Value expected[] = { Value(), Value(owner, &owner->first), Value(owner, &owner->second), Value(owner, nullptr) };
        LockFreeHashMap<int, Value> map(2);
        for (int key = 0; key < 4; ++key)
        {
            check(map.set(key, expected[key]), "shared export fixture publication");
        }
        exported = map.values();
        check(exported.size() == 4, "shared export complete cardinality");
        bool seen[] = { false, false, false, false };
        for (size_t index = 0; index < exported.size(); ++index)
        {
            bool found = false;
            for (size_t candidate = 0; candidate < 4; ++candidate)
            {
                // get相同不足以区分无拥有者的空指针和仍保活的空别名。
                if (!seen[candidate] && exported[index].get() == expected[candidate].get() &&
                    !exported[index].owner_before(expected[candidate]) && !expected[candidate].owner_before(exported[index]))
                {
                    seen[candidate] = true;
                    found = true;
                    break;
                }
            }
            check(found, "shared export preserves both stored address and ownership without duplicates");
        }
        map.clearAndFreeMemory();
    }
    check(!lifetime.expired(), "shared export owns values beyond map and input destruction");
    int sum = 0;
    for (size_t index = 0; index < exported.size(); ++index)
    {
        if (exported[index])
        {
            sum += *exported[index];
        }
    }
    check(sum == 78, "shared export keeps both aliased subobjects alive");
    exported.clear();
    check(lifetime.expired(), "last exported owner releases resource");

    // 单记录也能完成导出；只拒绝载荷数组，返回对象自身的调试代理遵循公开异常合同。
    LockFreeHashMapPoolConfig exportConfig;
    exportConfig.protectionRecordCount = 1;
    LockFreeHashMap<int, Value> singleRecord(4, exportConfig);
    Value singleOwner = std::make_shared<int>(53);
    for (int key = 0; key < 3; ++key)
    {
        check(singleRecord.set(key, singleOwner), "single record export fixture");
    }
    std::vector<Value> result;
    LockFreeHashMapPoolTestAllocation::s_failSize.store(3 * sizeof(Value));
    LockFreeHashMapPoolTestAllocation::s_fail.store(0);
    try
    {
        result = singleRecord.values();
    }
    catch (...)
    {
        LockFreeHashMapPoolTestAllocation::s_fail.store(-1);
        LockFreeHashMapPoolTestAllocation::s_failSize.store(0);
        throw;
    }
    bool injected = LockFreeHashMapPoolTestAllocation::s_fail.load() < 0;
    LockFreeHashMapPoolTestAllocation::s_fail.store(-1);
    LockFreeHashMapPoolTestAllocation::s_failSize.store(0);
    check(injected && result.empty() && singleOwner.use_count() == 4,
        "export payload allocation failure is caught and leaks no owner");
    result = singleRecord.values();
    check(result.size() == 3 && result[0] == singleOwner && result[1] == singleOwner && result[2] == singleOwner,
        "single protection record supports a complete export after failure");
    result.clear();
    check(singleOwner.use_count() == 4, "successful export releases all copied owners");
    int64_t beforeEmpty = LockFreeHashMapPoolTestAllocation::s_live.load();
    bool emptyAllocates = false;
    {
        std::vector<Value> empty;
        emptyAllocates = LockFreeHashMapPoolTestAllocation::s_live.load() > beforeEmpty;
    }
    if (emptyAllocates)
    {
        // v120 Debug的空vector本身申请代理，接口已明确允许返回对象构造异常传出。
        bool caught = false;
        LockFreeHashMapPoolTestAllocation::s_fail.store(0);
        try
        {
            result = singleRecord.values();
        }
        catch (const std::bad_alloc&)
        {
            caught = true;
        }
        catch (...)
        {
            LockFreeHashMapPoolTestAllocation::s_fail.store(-1);
            throw;
        }
        LockFreeHashMapPoolTestAllocation::s_fail.store(-1);
        check(caught && result.empty() && singleOwner.use_count() == 4,
            "return vector construction failure obeys the documented exception boundary");
    }
    // 长字符串键需要真实申请；键申请失败不得遗留已构造共享值的引用或占住固定槽。
    std::string firstKey(128, 'a'), secondKey(128, 'b');
    Value retained = std::make_shared<int>(59);
    LockFreeHashMapPoolConfig config;
    config.initialCapacity = 2;
    config.maximumCapacity = 2;
    config.directoryCapacity = 2;
    LockFreeHashMap<std::string, Value> strings(2, config);
    check(strings.status() == LockFreeHashMapOk, "long string and shared value fixture");
    for (size_t attempt = 0; attempt < 4; ++attempt)
    {
        LockFreeHashMapResult failed;
        LockFreeHashMapPoolTestAllocation::s_failAll.store(true);
        try
        {
            failed = strings.trySet(firstKey, retained);
        }
        catch (...)
        {
            LockFreeHashMapPoolTestAllocation::s_failAll.store(false);
            throw;
        }
        LockFreeHashMapPoolTestAllocation::s_failAll.store(false);
        check(!failed.m_committed && failed.m_status == LockFreeHashMapNoMemory &&
            strings.empty() && retained.use_count() == 1, "string allocation failure releases partial shared node");
    }
    check(strings.set(firstKey, retained) && strings.set(secondKey, retained) && retained.use_count() == 3,
        "long string failure leaves both fixed slots reusable");
    strings.clearAndFreeMemory();
    check(retained.use_count() == 1, "string shared nodes release their references");
}

void LockFreeHashMapRewriteTests::model()
{
    LockFreeHashMap<int, int, LockFreeHashMapRewriteHash> map(7);
    std::map<int, int> reference;
    check(map.status() == LockFreeHashMapOk && map.bucketCount() == 8, "normalized buckets and initialized map");
    uint32_t random = 873421;
    for (size_t round = 0; round < 40000; ++round)
    {
        random = random * 1664525U + 1013904223U;
        int key = int((random >> 8) % 67);
        int value = int(random & 0x7fffffff);
        switch (random % 7)
        {
        case 0:
        case 1:
        {
            check(map.set(key, value), "model set commits");
            reference[key] = value;
            break;
        }
        case 2:
        {
            bool inserted = reference.insert(std::make_pair(key, value)).second;
            check(map.insert(key, value) == inserted, "model conditional insert agrees");
            break;
        }
        case 3:
        {
            check(map.erase(key) == (reference.erase(key) != 0), "model erase agrees");
            break;
        }
        case 4:
        {
            int observed = -1;
            std::map<int, int>::const_iterator found = reference.find(key);
            check(map.get(key, observed) == (found != reference.end()), "model find presence agrees");
            check(found == reference.end() || observed == found->second, "model find value agrees");
            break;
        }
        case 5:
        {
            if ((round & 63) == 0)
            {
                map.clear();
                reference.clear();
            }
            break;
        }
        default:
        {
            check(map.contains(key) == (reference.count(key) != 0), "model contains agrees");
            break;
        }
        }
        check(map.count() == reference.size(), "quiescent sharded count is exact");
        if ((round & 127) == 0)
        {
            std::map<int, int> observed;
            // 回调将当前弱快照复制到独立参考容器，并检查没有重复键
            LockFreeHashMapResult result = map.forEachResult([&](const int& item, const int& data)
            {
                check(observed.insert(std::make_pair(item, data)).second, "model traversal no duplicate keys");
            });
            check(result.m_status == LockFreeHashMapOk && observed == reference, "complete collision traversal agrees with model");
        }
    }
    map.clear();
    LockFreeHashMapPoolStats beforeIdle = {}, afterIdle = {};
    check(map.poolStats(beforeIdle).m_status == LockFreeHashMapOk, "zero maintenance baseline observed");
    check(map.maintain(0).m_status == LockFreeHashMapOk && map.poolStats(afterIdle).m_status == LockFreeHashMapOk &&
        beforeIdle.capacity == afterIdle.capacity && beforeIdle.occupied == afterIdle.occupied &&
        beforeIdle.retired == afterIdle.retired, "zero maintenance performs no cleanup or reclamation");
    map.maintain(8);
    map.maintain(8);
    LockFreeHashMapPoolStats stats = {};
    check(map.poolStats(stats).m_status == LockFreeHashMapOk && stats.occupied == 0 && stats.retired == 0, "clear cleanup reclaims every old node");
    map.clearAndFreeMemory();
    check(map.set(1, 2) && map.value(1, 0) == 2, "reuse after explicit free");
}

void LockFreeHashMapRewriteTests::versions()
{
    typedef LockFreeHashMap<int, std::shared_ptr<int>> Map;
    Map map(1), other(1);
    Map::Snapshot first, winner, second, taken;
    std::shared_ptr<int> object = std::make_shared<int>(7);
    std::weak_ptr<int> weak = object;
    check(map.insertIfAbsent(1, object, first).m_committed, "first conditional insert commits");
    check(map.insertIfAbsent(1, std::make_shared<int>(9), winner).m_status == LockFreeHashMapExists && winner.m_value &&
        *winner.m_value && **winner.m_value == 7, "loser observes actual winner");
    check(map.compareExchange(1, first.m_version, object, &second).m_committed && second.m_version.m_sequence != first.m_version.m_sequence, "equal value gets distinct entry identity");
    check(!map.eraseIf(1, first.m_version).m_committed && !map.compareExchange(1, first.m_version, object).m_committed, "stale identity cannot modify replacement");
    check(other.insert(1, object) && other.eraseIf(1, second.m_version).m_status == LockFreeHashMapVersionMismatch, "cross-instance token refused");
    check(map.take(1, taken).m_committed && !map.contains(1) && taken.m_value && *taken.m_value &&
        **taken.m_value == 7, "take owns value independently");
    check(map.insert(1, object) && !map.eraseIf(1, second.m_version).m_committed, "reinsert rejects old identity");
    map.clear();
    check(map.insert(1, object) && !map.eraseIf(1, second.m_version).m_committed, "clear rejects old identity");
    map.clearAndFreeMemory();
    other.clearAndFreeMemory();
    object.reset();
    first = Map::Snapshot();
    winner = Map::Snapshot();
    second = Map::Snapshot();
    check(!weak.expired() && **taken.m_value == 7, "taken reference survives map cleanup");
    taken = Map::Snapshot();
    check(weak.expired(), "business object released exactly at final owner");
    LockFreeHashMap<int, int> bounded(1);
    LockFreeHashMapPoolConfig config;
    config.initialCapacity = 2;
    config.maximumCapacity = 2;
    check(bounded.configurePool(config).m_status == LockFreeHashMapOk && bounded.insert(1, 10) && bounded.insert(2, 20), "two entries occupy exactly two slots");
    check(bounded.trySet(3, 30).m_status == LockFreeHashMapCapacityLimit, "full budget reports capacity limit");
    check(bounded.erase(1) && bounded.set(3, 30) && bounded.count() == 2, "full pool deletion needs no allocation and permits reuse");
}

void LockFreeHashMapRewriteTests::identityCaches()
{
    typedef LockFreeHashMapTestState::IntMap Map;
    Map::Snapshot first, second;
    std::weak_ptr<const LockFreeHashMapIdentity> original;
    {
        LockFreeHashMapPoolConfig config;
        config.protectionRecordCount = 2;
        Map map(16, config);
        check(map.set(1, 7), "identity cache fixture populated");
        original = LockFreeHashMapTestState::identity(map);
        LockFreeHashMapEpoch::remember(0);
        check(map.find(1, first).m_status == LockFreeHashMapOk && first.m_value && *first.m_value == 7,
            "first record delivers complete snapshot");
        LockFreeHashMapEpoch::remember(1);
        check(map.find(1, second).m_status == LockFreeHashMapOk && second.m_value && *second.m_value == 7,
            "second record delivers complete snapshot");
        check(first.m_version.m_instance == second.m_version.m_instance &&
            first.m_version.m_sequence == second.m_version.m_sequence &&
            (first.m_version.m_instance.owner_before(second.m_version.m_instance) ||
            second.m_version.m_instance.owner_before(first.m_version.m_instance)),
            "independent reference counters retain the same complete instance and entry identity");
        LockFreeHashMapEpoch& domain = *LockFreeHashMapTestState::storage(map).epoch();
        check(domain.record(0)->m_snapshotBytes.load() == sizeof(LockFreeHashMapIdentityReference) &&
            domain.record(1)->m_snapshotBytes.load() == sizeof(LockFreeHashMapIdentityReference),
            "exactly two bounded identity owners are accounted");
        check(map.compareExchange(1, first.m_version, 8).m_committed &&
            map.eraseIf(1, second.m_version).m_status == LockFreeHashMapVersionMismatch,
            "tokens from different records obey identical conditional version checks");
        map.clearAndFreeMemory();
        check(map.configurePool(config).m_status == LockFreeHashMapOk && map.set(1, 9) &&
            map.eraseIf(1, first.m_version).m_status == LockFreeHashMapVersionMismatch,
            "reconfigured records cannot revive an old entry token");
        Map other(16, config);
        check(other.set(1, 9) && other.eraseIf(1, first.m_version).m_status == LockFreeHashMapVersionMismatch,
            "indirect identity owner cannot cross instance boundaries");
    }
    check(!original.expired() && *first.m_value == 7 && *second.m_value == 7,
        "snapshots keep the original identity and copied values alive after all maps and records are destroyed");
    first = Map::Snapshot();
    check(!original.expired(), "remaining alias still owns the original identity");
    second = Map::Snapshot();
    check(original.expired(), "last alias releases the original identity without retaining a record or map");
    for (int64_t point = 0; point < 3; ++point)
    {
        LockFreeHashMapPoolConfig config;
        config.protectionRecordCount = 1;
        Map map(16, config);
        check(map.set(1, 7), "optional identity allocation failure fixture populated");
        Map::Snapshot output;
        LockFreeHashMapPoolTestAllocation::s_fail.store(point);
        LockFreeHashMapResult result = map.find(1, output);
        LockFreeHashMapPoolTestAllocation::s_fail.store(-1);
        if (point == 0)
        {
            check(result.m_status == LockFreeHashMapNoMemory && !output.m_value,
                "required snapshot value allocation failure remains explicit");
        }
        else
        {
            check(result.m_status == LockFreeHashMapOk && output.m_value && *output.m_value == 7 &&
                output.m_version.m_instance == LockFreeHashMapTestState::identity(map) &&
                map.eraseIf(1, output.m_version).m_committed,
                "optional owner allocation failure falls back to a valid ordinary identity token");
        }
    }
}

void LockFreeHashMapRewriteTests::stringValues()
{
    LockFreeHashMapPoolConfig config;
    config.protectionRecordCount = 1;
    LockFreeHashMap<int, std::string> map(8, config);
    const std::string values[] = { std::string(), std::string("short"),
        std::string("a\0b\0c", 5), std::string(4096, 'x'), std::string("z") };
    std::string output("initial");
    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); ++i)
    {
        check(map.set(1, values[i]) && map.get(1, output) && output == values[i] && output.size() == values[i].size(),
            "string read preserves the complete range across empty, short, embedded-zero, long and shrinking values");
    }
    check(!map.get(2, output) && output == "z", "missing string read preserves previous output");
    check(map.set(1, values[3]), "string allocation failure fixture stores a long value");
    // 标准库可能在首次申请失败后重试；持续故障覆盖完整赋值，并以直接标准赋值作独立对照。
    // get不承诺用户赋值异常时输出不变；v120的字符串两次申请失败会清空原输出。
    std::string referenceOutput("hold");
    bool referenceFailed = false;
    LockFreeHashMapPoolTestAllocation::s_failAll.store(true);
    try
    {
        referenceOutput = values[3];
    }
    catch (const std::bad_alloc&)
    {
        referenceFailed = true;
    }
    catch (...)
    {
        LockFreeHashMapPoolTestAllocation::s_failAll.store(false);
        throw;
    }
    LockFreeHashMapPoolTestAllocation::s_failAll.store(false);
    std::string shortOutput("hold");
    bool copied = false;
    LockFreeHashMapPoolTestAllocation::s_failAll.store(true);
    try
    {
        copied = map.get(1, shortOutput);
    }
    catch (...)
    {
        LockFreeHashMapPoolTestAllocation::s_failAll.store(false);
        throw;
    }
    LockFreeHashMapPoolTestAllocation::s_failAll.store(false);
    check(referenceFailed && !copied && shortOutput == referenceOutput,
        "persistent string allocation failure is caught and output matches direct standard assignment");
    check(map.get(1, shortOutput) && shortOutput == values[3], "string read recovers after allocation failure without damaging the entry");
    const std::string equalLength(4096, 'y');
    check(map.set(1, equalLength), "equal-length string replacement fixture ready");
#if defined(_CPPLIB_VER) || defined(_LIBCPP_VERSION) || (defined(__GLIBCXX__) && defined(_GLIBCXX_USE_CXX11_ABI) && _GLIBCXX_USE_CXX11_ABI)
    // 只有明确启用无分配字符覆盖的后端要求此成本合同，其他标准库只检查完整值交付。
    LockFreeHashMapPoolTestAllocation::s_failAll.store(true);
#endif
    try
    {
        copied = map.get(1, shortOutput);
    }
    catch (...)
    {
        LockFreeHashMapPoolTestAllocation::s_failAll.store(false);
        throw;
    }
    LockFreeHashMapPoolTestAllocation::s_failAll.store(false);
    check(copied && shortOutput == equalLength && shortOutput.size() == equalLength.size() &&
        shortOutput.c_str()[shortOutput.size()] == '\0',
        "equal-length character replacement preserves size, content and terminator under the backend allocation contract");
    const size_t lengths[] = { 1, 15, 16, 31, 32, 63, 64, 65, 127, 128, 129, 255, 256, 257 };
    for (size_t i = 0; i < sizeof(lengths) / sizeof(lengths[0]); ++i)
    {
        // 覆盖短字符串及常见复制分块边界；期望由输入逐字节构造，含内嵌零和高位字节。
        std::string expected(lengths[i], '\0');
        for (size_t offset = 0; offset < expected.size(); ++offset)
        {
            expected[offset] = static_cast<char>((offset * 37 + i * 19) & 255);
        }
        std::string observed(lengths[i], 'q');
        check(map.set(1, expected), "string copy boundary fixture ready");
#if defined(_CPPLIB_VER) || defined(_LIBCPP_VERSION) || (defined(__GLIBCXX__) && defined(_GLIBCXX_USE_CXX11_ABI) && _GLIBCXX_USE_CXX11_ABI)
        LockFreeHashMapPoolTestAllocation::s_failAll.store(true);
#endif
        try
        {
            copied = map.get(1, observed);
        }
        catch (...)
        {
            LockFreeHashMapPoolTestAllocation::s_failAll.store(false);
            throw;
        }
        LockFreeHashMapPoolTestAllocation::s_failAll.store(false);
        check(copied && observed == expected && observed.size() == lengths[i] &&
            observed.c_str()[observed.size()] == '\0',
            "equal-length binary string copies preserve all bytes at storage and copy boundaries");
    }
    map.clear();
    check(!map.get(1, shortOutput) && shortOutput == equalLength, "clear hides the entry without modifying failed read output");
}

void LockFreeHashMapRewriteTests::exceptions()
{
    typedef LockFreeHashMap<int, LockFreeHashMapRewriteValue, LockFreeHashMapRewriteHash, LockFreeHashMapRewriteEqual> Map;
    Map map(1);
    LockFreeHashMapRewriteValue one(1), two(2);
    check(map.set(1, one), "exception fixture initialized");
    LockFreeHashMapRewriteHash::s_throw.store(true);
    LockFreeHashMapResult hash = map.trySet(1, two);
    LockFreeHashMapRewriteHash::s_throw.store(false);
    check(hash.m_status == LockFreeHashMapUserException && !hash.m_committed, "hash exception does not commit");
    LockFreeHashMapRewriteEqual::s_throw.store(true);
    LockFreeHashMapResult equal = map.trySet(1, two);
    LockFreeHashMapRewriteEqual::s_throw.store(false);
    check(equal.m_status == LockFreeHashMapUserException && !equal.m_committed, "comparison exception is not absence");
    // 默认键的存在性查询不赋值；读取复杂值仍须捕获赋值异常，不能误走平凡复制路径。
    LockFreeHashMap<int, LockFreeHashMapRewriteValue> standard(1);
    check(standard.set(1, one), "standard-key throwing-value fixture initialized");
    LockFreeHashMapRewriteValue delivered(77);
    LockFreeHashMapRewriteValue::s_throw.store(true);
    bool exists = standard.contains(1);
    bool missing = standard.contains(2);
    bool copied = standard.get(1, delivered);
    LockFreeHashMapRewriteValue::s_throw.store(false);
    check(exists && !missing && !copied && delivered.m_value == 77,
        "standard-key contains avoids user assignment while get captures its exception");
    check(standard.get(1, delivered) && delivered.m_value == 1,
        "failed user assignment releases protection and preserves the stored value");
    LockFreeHashMapRewriteValue::s_throw.store(true);
    LockFreeHashMapResult copy = map.trySet(1, two);
    Map::Snapshot output;
    LockFreeHashMapResult take = map.take(1, output);
    bool pairsEmpty = map.toVector().empty();
    LockFreeHashMapRewriteValue::s_throw.store(false);
    check(copy.m_status == LockFreeHashMapUserException && !copy.m_committed && !take.m_committed && !output.m_value, "copy failure preserves entry and output");
    check(pairsEmpty, "pair construction failure leaves no partial output");
    check(map.find(1, output).m_status == LockFreeHashMapOk && output.m_value && output.m_value->m_value == 1, "old entry survives all user failures");
    LockFreeHashMapPoolTestAllocation::s_fail.store(0);
    try
    {
        pairsEmpty = map.toVector().empty();
    }
    catch (const std::bad_alloc&)
    {
        // v120 Debug 连空向量也会申请迭代器代理；返回容器构造异常按公开契约由调用方处理
        pairsEmpty = true;
    }
    LockFreeHashMapPoolTestAllocation::s_fail.store(-1);
    check(pairsEmpty && map.count() == 1, "pair allocation failure preserves source map");
    {
        std::vector<std::pair<int, LockFreeHashMapRewriteValue>> pairs = map.toVector();
        check(pairs.size() == 1 && pairs[0].first == 1 && pairs[0].second.m_value == 1, "pair export recovers after allocation and construction failures");
    }
    Map::Snapshot preserved = output;
    // 已有输出不能因复制或分配失败而丢失值、实例身份或版本；逐字段独立核对。
    LockFreeHashMapRewriteValue::s_throw.store(true);
    LockFreeHashMapResult failedFindCopy = map.find(1, output);
    LockFreeHashMapRewriteValue::s_throw.store(false);
    check(failedFindCopy.m_status == LockFreeHashMapUserException && !failedFindCopy.m_committed &&
        output.m_value == preserved.m_value && output.m_version.m_instance == preserved.m_version.m_instance &&
        output.m_version.m_sequence == preserved.m_version.m_sequence, "failed find copy preserves complete existing snapshot");
    LockFreeHashMapPoolTestAllocation::s_fail.store(0);
    LockFreeHashMapResult failedFindAllocation = map.find(1, output);
    LockFreeHashMapPoolTestAllocation::s_fail.store(-1);
    check(failedFindAllocation.m_status == LockFreeHashMapNoMemory && !failedFindAllocation.m_committed &&
        output.m_value == preserved.m_value && output.m_version.m_instance == preserved.m_version.m_instance &&
        output.m_version.m_sequence == preserved.m_version.m_sequence, "failed find allocation preserves complete existing snapshot");
    check(map.find(2, output).m_status == LockFreeHashMapNotFound && output.m_value == preserved.m_value &&
        output.m_version.m_instance == preserved.m_version.m_instance &&
        output.m_version.m_sequence == preserved.m_version.m_sequence, "missing find preserves complete existing snapshot");
    LockFreeHashMapPoolTestAllocation::s_fail.store(0);
    LockFreeHashMapResult allocation = map.compareExchange(1, output.m_version, two, &output);
    LockFreeHashMapPoolTestAllocation::s_fail.store(-1);
    check(!allocation.m_committed && allocation.m_status == LockFreeHashMapNoMemory && output.m_value == preserved.m_value, "output allocation failure occurs before conditional commit");
    // 回调异常必须释放保护，返回完成数而不将失败伪装成成功遍历
    LockFreeHashMapResult callback = map.forEachResult([](const int& key, const LockFreeHashMapRewriteValue& value)
    {
        (void)key;
        (void)value;
        throw 35;
    });
    check(callback.m_status == LockFreeHashMapUserException && callback.m_count == 0 && map.set(1, two), "callback exception leaves map usable");
    for (int64_t point = 0; point < 9; ++point)
    {
        int64_t before = LockFreeHashMapPoolTestAllocation::s_live.load();
        {
            LockFreeHashMapPoolTestAllocation::s_fail.store(point);
            LockFreeHashMap<int, int> failing;
            LockFreeHashMapPoolTestAllocation::s_fail.store(-1);
            check(failing.status() == LockFreeHashMapNoMemory, "every constructor allocation failure reported");
        }
        check(LockFreeHashMapPoolTestAllocation::s_live.load() == before, "failed constructor releases every allocation");
    }
}

void LockFreeHashMapRewriteTests::paused()
{
    typedef LockFreeHashMap<int, LockFreeHashMapRewriteValue> Map;
    Map map(1);
    LockFreeHashMapRewriteValue oldValue(71), newValue(92);
    check(map.set(1, oldValue), "paused reader fixture initialized");
    Map::Snapshot observed;
    LockFreeHashMapResult result;
    LockFreeHashMapRewriteValue::s_entered.store(false);
    LockFreeHashMapRewriteValue::s_resume.store(false);
    LockFreeHashMapRewriteValue::s_pause.store(71);
    // find 在取得节点后进入值复制暂停点；主线程同时替换、清空并维护
    LockFreeHashMapTestThread reader([&]()
    {
        result = map.find(1, observed);
    });
    try
    {
        while (!LockFreeHashMapRewriteValue::s_entered.load())
        {
            std::this_thread::yield();
        }
        check(map.set(1, newValue), "replacement progresses while reader paused");
        map.clear();
        check(map.set(1, newValue), "new generation publishes while reader paused");
        map.maintain(1);
        LockFreeHashMapPoolStats stats = {};
        check(map.poolStats(stats).m_status == LockFreeHashMapOk, "paused snapshot metrics available");
        check(stats.retired != 0, "old storage retained while copy is protected");
        LockFreeHashMapRewriteValue::s_resume.store(true);
        reader.join();
    }
    catch (...)
    {
        LockFreeHashMapRewriteValue::s_resume.store(true);
        reader.join();
        throw;
    }
    check(result.m_status == LockFreeHashMapOk && observed.m_value && observed.m_value->m_value == 71 &&
        !map.eraseIf(1, observed.m_version).m_committed, "paused snapshot keeps old value and cannot erase new generation");
    map.maintain(1);
    LockFreeHashMapPoolStats stats = {};
    check(map.poolStats(stats).m_status == LockFreeHashMapOk, "resumed snapshot metrics available");
    check(stats.retired == 0 && stats.occupied == 1, "paused storage reclaimed after reader exits");
}

void LockFreeHashMapRewriteTests::contention()
{
    typedef LockFreeHashMap<int, int, LockFreeHashMapRewriteHash> Map;
    Map map(1);
    std::atomic<int> winners(0), ready(0);
    std::atomic<bool> start(false);
    int observedWinners[8] = {};
    std::vector<LockFreeHashMapTestThread> workers;
    workers.reserve(8);
    try
    {
        for (int thread = 0; thread < 8; ++thread)
        {
            // 屏障后竞争同键，候选不同但所有失败者必须读到同一胜者
            workers.push_back(LockFreeHashMapTestThread([&, thread]()
            {
                ready.fetch_add(1);
                while (!start.load())
                {
                    std::this_thread::yield();
                }
                Map::Snapshot result;
                LockFreeHashMapResult inserted = map.insertIfAbsent(1, thread, result);
                check((inserted.m_status == LockFreeHashMapOk || inserted.m_status == LockFreeHashMapExists) && result.m_value &&
                    *result.m_value >= 0 && *result.m_value < 8, "all insert contenders obtain actual entry");
                observedWinners[thread] = *result.m_value;
                if (inserted.m_committed)
                {
                    winners.fetch_add(1);
                }
                for (int operation = 0; operation < 2000; ++operation)
                {
                    for (;;)
                    {
                        Map::Snapshot current;
                        check(map.find(2, current).m_status == LockFreeHashMapOk && current.m_value, "CAS counter always present");
                        LockFreeHashMapResult changed = map.compareExchange(2, current.m_version, *current.m_value + 1);
                        if (changed.m_committed)
                        {
                            break;
                        }
                        check(changed.m_status == LockFreeHashMapVersionMismatch || changed.m_status == LockFreeHashMapReservationBusy, "CAS loser has a retryable conflict");
                    }
                }
            }));
        }
        check(map.set(2, 0), "CAS counter initialized");
        while (ready.load() != 8)
        {
            std::this_thread::yield();
        }
        start.store(true);
    }
    catch (...)
    {
        // 创建后续线程失败也先放开已创建线程的测试屏障，回栈才能完成连接
        start.store(true);
        throw;
    }
    for (size_t i = 0; i < workers.size(); ++i)
    {
        workers[i].join();
    }
    check(winners.load() == 1 && map.value(2, -1) == 16000 && map.count() == 2, "one insert winner and exact conditional increment total");
    for (int thread = 0; thread < 8; ++thread)
    {
        check(observedWinners[thread] == map.value(1, -1), "同键创建的所有失败者观察到同一个实际胜者");
    }
    workers.clear();
    winners.store(0);
    for (int thread = 0; thread < 8; ++thread)
    {
        // 完成与取消都使用原子 take，只有一个调用取得最终责任
        workers.push_back(LockFreeHashMapTestThread([&]()
        {
            Map::Snapshot taken;
            LockFreeHashMapResult result = map.take(2, taken);
            if (result.m_committed)
            {
                check(taken.m_value && *taken.m_value == 16000, "taken terminal value is intact");
                winners.fetch_add(1);
            }
            else
            {
                check(result.m_status == LockFreeHashMapNotFound, "take loser sees absence");
            }
        }));
    }
    for (size_t i = 0; i < workers.size(); ++i)
    {
        workers[i].join();
    }
    check(winners.load() == 1 && map.count() == 1, "one terminal take winner");
}

void LockFreeHashMapRewriteTests::collisions()
{
    LockFreeHashMap<int, uint64_t, LockFreeHashMapRewriteHash> map(1);
    std::vector<LockFreeHashMapTestThread> workers;
    workers.reserve(8);
    for (size_t thread = 0; thread < 8; ++thread)
    {
        // 每个线程持有独立键区间；其他线程的物理清理仍会修改同一条碰撞链
        workers.push_back(LockFreeHashMapTestThread([&, thread]()
        {
            bool present[8] = {};
            for (size_t round = 0; round < 10000; ++round)
            {
                int key = int(thread * 8 + round % 8);
                uint64_t value = (uint64_t(key) << 32) | round;
                if (round % 5 == 0)
                {
                    check(map.erase(key) == present[round % 8] && !map.contains(key), "独占键删除结果与独立存在状态一致");
                    present[round % 8] = false;
                }
                else
                {
                    check(map.set(key, value), "collision write commits without dropped nodes");
                    present[round % 8] = true;
                    uint64_t observed = 0;
                    check(map.get(key, observed) && observed == value, "collision read agrees with sole key owner");
                }
            }
        }));
    }
    for (size_t i = 0; i < workers.size(); ++i)
    {
        workers[i].join();
    }
    map.clear();
    for (int key = 0; key < 64; ++key)
    {
        check(map.insert(key, uint64_t(key)), "complete collision table refilled after clear");
    }
    std::set<int> keys;
    // 静止期核对所有键都存在一次，没有被物理摘除丢弃
    LockFreeHashMapResult result = map.forEachResult([&](const int& key, const uint64_t& value)
    {
        check(value == uint64_t(key) && keys.insert(key).second, "final collision set contains no stale or duplicate entries");
    });
    check(result.m_status == LockFreeHashMapOk && keys.size() == 64 && map.count() == 64, "final collision table agrees with independent key set");
}

LockFreeHashMapValueCacheEntry& LockFreeHashMapValueCacheTestAccess::first(
    LockFreeHashMapValueCache<uint64_t, uint64_t, std::equal_to<uint64_t>>& cache)
{
    return LockFreeHashMapTestState::first(cache);
}

void LockFreeHashMapRewriteTests::scalarCache()
{
    LockFreeHashMapValueCache<uint64_t, uint64_t, std::equal_to<uint64_t>> cache;
    check(cache.initialize(3) == LockFreeHashMapInvalidArgument && cache.count() == 0, "cache rejects non power of two");
    LOCKFREEHASHMAP_TEST_ALIGN_64 std::atomic<uint64_t> generation(1);
    uint64_t output = 11;
    check(!cache.lookup(1, generation, &output) && output == 11, "disabled cache preserves output with atomic generation");
    LockFreeHashMapValueCache<std::string, uint64_t, std::equal_to<std::string>> disabled;
    check(!disabled.lookup(std::string("key"), generation, &output) && output == 11,
        "nonintegral cache preserves output with atomic generation");
    {
        LockFreeHashMapTestState::ScalarCache first, second, empty;
        const size_t excessive = size_t(1) << ((std::numeric_limits<size_t>::digits) - 1);
        check(first.initialize(excessive) == LockFreeHashMapInvalidArgument && first.count() == 0 && first.bytes() == 0,
            "cache rejects overflowing allocation size before allocating");
        LockFreeHashMapStatus failed = LockFreeHashMapOk;
        LockFreeHashMapPoolTestAllocation::s_failAll.store(true);
        try
        {
            failed = first.initialize(2);
        }
        catch (...)
        {
            LockFreeHashMapPoolTestAllocation::s_failAll.store(false);
            throw;
        }
        LockFreeHashMapPoolTestAllocation::s_failAll.store(false);
        check(failed == LockFreeHashMapNoMemory && first.count() == 0 && first.bytes() == 0 &&
            first.initialize(2) == LockFreeHashMapOk && second.initialize(8) == LockFreeHashMapOk,
            "failed aligned allocation leaves an empty reusable cache");
        first.invalidate(1, 1);
        first.publish(1, 17, 1, 1);
        second.invalidate(2, 2);
        second.publish(2, 29, 1, 2);
        check(first.initialize(4) == LockFreeHashMapInvalidArgument && first.lookup(1, generation, &output) && output == 17,
            "double initialization preserves the existing aligned allocation and value");
        first.swap(second);
        check(first.count() == 8 && second.count() == 2 && first.lookup(2, generation, &output) && output == 29 &&
            second.lookup(1, generation, &output) && output == 17,
            "cache swap transfers allocation ownership together with complete entries");
        empty.swap(first);
        check(first.count() == 0 && first.bytes() == 0 && empty.count() == 8 &&
            empty.lookup(2, generation, &output) && output == 29,
            "swapping an aligned cache into an empty cache transfers exactly one allocation");
        output = 11;
    }
    check(cache.initialize(1) == LockFreeHashMapOk, "one cache entry initialized");
    uint64_t key = UINT64_C(0xfedcba9876543210), value = UINT64_C(0x8765432100000001);
    check(!cache.lookup(key, 1, &output) && output == 11, "empty cache keeps output unchanged");
    cache.invalidate(key, 1);
    cache.publish(key, value, 1, 1);
    check(cache.lookup(key, 1, &output) && output == value, "full width key and value preserved");
    check(cache.lookup(key, generation, &output) && output == value,
        "atomic generation reads a complete published copy");
    generation.store(2);
    output = 11;
    check(!cache.lookup(key, generation, &output) && output == 11,
        "atomic generation rejects preclear copy without modifying output");
    generation.store(0);
    check(!cache.lookup(key, generation, &output) && output == 11,
        "exhausted atomic generation never returns a copy");
    generation.store(1);
    check(cache.lookup(key, generation, static_cast<uint64_t*>(nullptr)),
        "atomic generation supports existence query without value output");
    check(!cache.lookup(key ^ (UINT64_C(1) << 48), 1, &output), "high key bits distinguish colliding cache keys");
    check(!cache.lookup(key, 2, &output), "clear generation rejects old cache");
    cache.invalidate(key, 2);
    cache.publish(key, value, 1, 1);
    check(!cache.lookup(key, 1, &output), "late publisher cannot restore old identity");
    cache.publish(key, value + 1, 1, 2);
    check(cache.lookup(key, 1, &output) && output == value + 1, "next identity published");
    LockFreeHashMapValueCacheEntry& entry = LockFreeHashMapValueCacheTestAccess::first(cache);
    entry.m_publishing.store(true);
    for (uint64_t version = 3; version < 100; ++version)
    {
        cache.invalidate(key, version);
        cache.publish(key, value + version, 1, version);
        check(!cache.lookup(key, 1, &output), "paused cache publisher does not block later invalidations");
    }
    entry.m_publishing.store(false);
    cache.publish(key, value + 2, 1, 3);
    check(!cache.lookup(key, 1, &output), "old publisher stays invalid after resuming");
    cache.publish(key, value + 99, 1, 99);
    check(cache.lookup(key, 1, &output) && output == value + 99, "current publisher resumes normally");
    cache.invalidate(key, 0);
    check(!cache.lookup(key, 1, &output), "delete invalidates without allocating a version");
    uint64_t maximum = (std::numeric_limits<uint64_t>::max)();
    cache.invalidate(key, maximum >> 1);
    cache.publish(key, value, maximum, maximum >> 1);
    check(cache.lookup(key, maximum, &output) && output == value, "last encodable identity and full generation supported");
    cache.invalidate(key, (maximum >> 1) + 1);
    cache.publish(key, value, maximum, (maximum >> 1) + 1);
    check(!cache.lookup(key, maximum, &output), "cache identity exhaustion disables copies without wrapping");

    LockFreeHashMapValueCache<int32_t, int32_t, std::equal_to<int32_t>> narrow;
    check(narrow.initialize(1) == LockFreeHashMapOk, "single-slot narrow cache forces collisions independently of indexing");
    const int32_t minimumKey = (std::numeric_limits<int32_t>::min)();
    int32_t narrowOutput = 99;
    narrow.invalidate(minimumKey, 1);
    narrow.publish(minimumKey, -12, 1, 1);
    check(narrow.lookup(minimumKey, 1, &narrowOutput) && narrowOutput == -12,
        "minimum signed key keeps all identity and payload bits");
    narrow.invalidate(0, 2);
    narrow.publish(0, 13, 1, 2);
    check(!narrow.lookup(minimumKey, 1, &narrowOutput) && narrowOutput == -12 &&
        narrow.lookup(0, 1, &narrowOutput) && narrowOutput == 13,
        "colliding minimum and zero keys cannot return each other's values");
    narrow.invalidate(-1, 3);
    narrow.publish(-1, -14, 1, 3);
    check(narrow.lookup(-1, 1, &narrowOutput) && narrowOutput == -14, "negative narrow key round trip");
    narrow.invalidate(7, 4);
    narrow.publish(-1, -14, 1, 3);
    check(!narrow.lookup(-1, 1, &narrowOutput) && narrowOutput == -14,
        "late negative publisher cannot cross a colliding positive-key invalidation");
    narrow.publish(7, 15, 1, 4);
    check(narrow.lookup(7, 1, &narrowOutput) && narrowOutput == 15 && !narrow.lookup(-1, 1, &narrowOutput),
        "positive colliding key publishes with its own complete identity");

    // 窄值对象表示、无符号最高位及混合位宽都通过完整输入/输出验证，不读取缓存编码。
    LockFreeHashMapValueCache<int16_t, int8_t, std::equal_to<int16_t>> bytes;
    check(bytes.initialize(1) == LockFreeHashMapOk, "small signed cache initialized");
    const int16_t shortKeys[] = { (std::numeric_limits<int16_t>::min)(), -1, 0, (std::numeric_limits<int16_t>::max)() };
    const int8_t byteValues[] = { (std::numeric_limits<int8_t>::min)(), -1, 0, (std::numeric_limits<int8_t>::max)() };
    for (size_t i = 0; i < 4; ++i)
    {
        int8_t actual = 42;
        bytes.invalidate(shortKeys[i], i + 1);
        bytes.publish(shortKeys[i], byteValues[i], maximum, i + 1);
        check(bytes.lookup(shortKeys[i], maximum, &actual) && actual == byteValues[i] &&
            bytes.lookup(shortKeys[i], maximum, static_cast<int8_t*>(nullptr)), "signed byte representation and full generation preserved");
        actual = 42;
        check(!bytes.lookup(shortKeys[(i + 1) % 4], maximum, &actual) && actual == 42,
            "small signed collision preserves missing output");
    }
    LockFreeHashMapValueCache<uint32_t, uint32_t, std::equal_to<uint32_t>> unsignedWords;
    check(unsignedWords.initialize(1) == LockFreeHashMapOk, "unsigned cache initialized");
    uint32_t word = 0;
    unsignedWords.invalidate(UINT32_MAX, 1);
    unsignedWords.publish(UINT32_MAX, UINT32_C(0x80000001), maximum, 1);
    check(unsignedWords.lookup(UINT32_MAX, maximum, &word) && word == UINT32_C(0x80000001),
        "unsigned key and value retain their high bits");
    LockFreeHashMapValueCache<uint32_t, uint64_t, std::equal_to<uint32_t>> wideValue;
    check(wideValue.initialize(1) == LockFreeHashMapOk, "mixed width value cache initialized");
    wideValue.invalidate(UINT32_MAX, 1);
    wideValue.publish(UINT32_MAX, value, maximum, 1);
    check(wideValue.lookup(UINT32_MAX, maximum, &output) && output == value,
        "narrow key never truncates a wide value");
    LockFreeHashMapValueCache<uint64_t, uint32_t, std::equal_to<uint64_t>> wideKey;
    check(wideKey.initialize(1) == LockFreeHashMapOk, "mixed width key cache initialized");
    wideKey.invalidate(key, 1);
    wideKey.publish(key, UINT32_MAX, maximum, 1);
    check(wideKey.lookup(key, maximum, &word) && word == UINT32_MAX &&
        !wideKey.lookup(key ^ (UINT64_C(1) << 48), maximum, &word) && word == UINT32_MAX,
        "narrow value never truncates a wide key");

    LockFreeHashMapPoolConfig signedConfig;
    signedConfig.lookupCacheCapacity = 8;
    LockFreeHashMap<int32_t, int32_t> signedMap(16, signedConfig);
    LockFreeHashMapPoolStats narrowStats = {};
    check(signedMap.poolStats(narrowStats).m_status == LockFreeHashMapOk &&
        narrowStats.lookupCacheEntries == 8 &&
        narrowStats.lookupCacheBytes == 8 * sizeof(LockFreeHashMapPackedValueCacheEntry) + 63 &&
        sizeof(LockFreeHashMapPackedValueCacheEntry) < sizeof(LockFreeHashMapValueCacheEntry),
        "narrow cache accounting reflects compact storage without the unused wide value");
    const int32_t signedKeys[] = { minimumKey, -1, 0, 1, (std::numeric_limits<int32_t>::max)(), 7, 8, 15 };
    for (size_t i = 0; i < sizeof(signedKeys) / sizeof(signedKeys[0]); ++i)
    {
        check(signedMap.set(signedKeys[i], static_cast<int32_t>(i + 42)), "signed map cache fixture insertion");
        for (size_t j = 0; j <= i; ++j)
        {
            check(signedMap.get(signedKeys[j], narrowOutput) && narrowOutput == static_cast<int32_t>(j + 42),
                "signed map preserves every key after cache collisions");
        }
    }

    for (int enabled = 0; enabled < 2; ++enabled)
    {
        LockFreeHashMap<int64_t, uint64_t> map(16);
        LockFreeHashMapPoolConfig config;
        config.lookupCacheCapacity = enabled ? 1 : 0;
        check(map.configurePool(config).m_status == LockFreeHashMapOk, "configure scalar cache mode");
        for (int64_t i = -1000; i <= 1000; ++i)
        {
            int64_t fullKey = i * INT64_C(0x100000001);
            uint64_t fullValue = static_cast<uint64_t>(fullKey) ^ UINT64_C(0xa5a5c3c37e7e1f1f);
            check(map.set(fullKey, fullValue), "signed wide key set");
            check(map.get(fullKey, output) && output == fullValue, "signed wide key lookup");
            check(map.erase(fullKey) && !map.contains(fullKey), "signed wide key erase invalidates cache");
        }
        LockFreeHashMapPoolStats stats = {};
        check(map.poolStats(stats).m_status == LockFreeHashMapOk && stats.lookupCacheEntries == size_t(enabled) &&
            stats.lookupCacheBytes == (enabled ? sizeof(LockFreeHashMapValueCacheEntry) + 63 : 0),
            "cache memory includes the allocation alignment allowance and is zero when disabled");
    }
}

void LockFreeHashMapRewriteTests::scalarStress()
{
    LockFreeHashMap<uint64_t, uint64_t> map(64);
    LockFreeHashMapPoolConfig config;
    config.lookupCacheCapacity = 64;
    check(map.configurePool(config).m_status == LockFreeHashMapOk, "stress scalar cache configured");
    std::vector<LockFreeHashMapTestThread> workers;
    workers.reserve(8);
    for (size_t thread = 0; thread < 8; ++thread)
    {
        workers.push_back(LockFreeHashMapTestThread([&, thread]()
        {
            for (uint64_t i = 0; i < 15000; ++i)
            {
                uint64_t key = ((i + thread) & 63) | UINT64_C(0xfedcba9800000000);
                uint64_t expected = key ^ UINT64_C(0xa5c37e1f5a3ce7f1);
                if (thread == 0 && i % 17 == 0)
                {
                    check(map.tryClear().m_committed, "concurrent scalar clear commits");
                }
                else if (i % 7 == 0)
                {
                    map.erase(key);
                }
                else if (i % 3 == 0)
                {
                    LockFreeHashMapResult result = map.trySet(key, expected, nullptr, true, false);
                    check(result.m_committed || result.m_status == LockFreeHashMapCleared, "write either commits or reports concurrent clear");
                }
                else
                {
                    uint64_t output = 0;
                    bool found = map.get(key, output);
                    check(!found || output == expected, "scalar cache never mixes different key value copies");
                }
            }
        }));
    }
    for (size_t i = 0; i < workers.size(); ++i)
    {
        workers[i].join();
    }
    map.clear();
    for (uint64_t key = 0; key < 64; ++key)
    {
        check(map.set(key, key + 1), "final scalar writes commit");
        uint64_t output = 0;
        check(map.get(key, output) && output == key + 1, "final scalar cache contains latest completed write");
    }
    check(map.count() == 64, "concurrent clear count settles exactly");
}

void LockFreeHashMapRewriteTests::narrowStress()
{
    LockFreeHashMapPoolConfig config;
    config.lookupCacheCapacity = 1;
    LockFreeHashMap<uint32_t, uint32_t> map(64, config);
    check(map.status() == LockFreeHashMapOk, "narrow collision fixture initialized");
    std::vector<LockFreeHashMapTestThread> workers;
    workers.reserve(8);
    for (uint32_t thread = 0; thread < 8; ++thread)
    {
        // 线程借用同一表，独立输入和输出；所有键争用一个副本格，值可由完整键独立计算。
        workers.push_back(LockFreeHashMapTestThread([&, thread]()
        {
            for (uint32_t i = 0; i < 15000; ++i)
            {
                uint32_t key = UINT32_C(0x80000000) | ((i + thread) & 63);
                uint32_t expected = key ^ UINT32_C(0xa5c37e1f);
                if (thread == 0 && i % 17 == 0)
                {
                    check(map.tryClear().m_committed, "narrow concurrent clear commits");
                }
                else if (i % 7 == 0)
                {
                    map.erase(key);
                }
                else if (i % 3 == 0)
                {
                    check(map.set(key, expected), "narrow concurrent write commits");
                }
                else
                {
                    uint32_t output = 0;
                    bool found = map.get(key, output);
                    check(found ? output == expected : output == 0, "narrow cache preserves complete key value or unchanged missing output");
                }
            }
        }));
    }
    for (size_t i = 0; i < workers.size(); ++i)
    {
        workers[i].join();
    }
    map.clear();
    for (uint32_t key = 0; key < 64; ++key)
    {
        uint32_t output = 0;
        check(map.set(key, ~key) && map.get(key, output) && output == ~key,
            "narrow final state matches latest completed write");
    }
    check(map.count() == 64, "narrow concurrent count settles exactly");
}

void LockFreeHashMapRewriteTests::largeTableLifecycle()
{
    const int32_t count = 65536;
    const int64_t baseline = LockFreeHashMapPoolTestAllocation::s_live.load();
    for (int32_t round = 0; round < 8; ++round)
    {
        {
            LockFreeHashMap<int32_t, int32_t> map(4096);
            for (int32_t key = 0; key < count; ++key)
            {
                check(map.set(key, key * 2 + round), "large table sequential prefill commits");
            }
            check(map.count() == size_t(count), "large table count equals independent key count");
            for (int32_t key = 0; key < count; ++key)
            {
                int32_t output = -1;
                check(map.get(key, output) && output == key * 2 + round, "large table prefill preserves every key");
                if ((key & 7) == 0)
                {
                    check(map.set(key, -key - round - 1), "large table replacement commits");
                }
            }
            for (int32_t key = 0; key < count; ++key)
            {
                int32_t output = 0;
                int32_t expected = (key & 7) == 0 ? -key - round - 1 : key * 2 + round;
                check(map.get(key, output) && output == expected, "large table replacement preserves complete contents");
            }
            if ((round & 1) == 0)
            {
                map.clear();
                for (int32_t key = 0; key < count; ++key)
                {
                    check(!map.contains(key), "large table clear hides every old key");
                }
                map.clearAndFreeMemory();
                LockFreeHashMapPoolStats stats = {};
                check(map.poolStats(stats).m_status == LockFreeHashMapOk && stats.capacity == 0 &&
                    stats.occupied == 0 && stats.retired == 0, "large table explicit release returns all pool capacity");
                check(map.set(round, round + 1), "large table accepts writes after explicit release");
            }
        }
        check(LockFreeHashMapPoolTestAllocation::s_live.load() == baseline, "large table destructor releases every allocation");
    }
}

LockFreeHashMapAlignmentFixture::LockFreeHashMapAlignmentFixture() :
m_prefix(0)
{

}

void LockFreeHashMapRewriteTests::aligned(const void* field, const char* name)
{
    bool valid = (reinterpret_cast<uintptr_t>(field) & 7) == 0;
    if (!valid)
    {
        std::cerr << "ALIGNMENT_FAILURE field=" << name << " address=" << field << std::endl;
    }
    check(valid, name);
}

void LockFreeHashMapRewriteTests::alignment()
{
    typedef LockFreeHashMapTestState::IntMap Map;
    typedef LockFreeHashMapPoolBlock<int> Block;
    std::cout << "ALIGNMENT_TYPES map=" << std::alignment_of<Map>::value <<
        " epoch=" << std::alignment_of<LockFreeHashMapEpoch>::value <<
        " record=" << std::alignment_of<LockFreeHashMapEpochRecord>::value <<
        " block=" << std::alignment_of<Block>::value <<
        " cache=" << std::alignment_of<LockFreeHashMapValueCacheEntry>::value << std::endl;
    // 先拒绝声明对齐不足的旧类型，不能为了复现而主动执行不满足原子后端前提的读写
    check(std::alignment_of<Map>::value >= 8 && std::alignment_of<Map::Storage>::value >= 8 &&
        std::alignment_of<LockFreeHashMapEpoch>::value >= 8 &&
        std::alignment_of<LockFreeHashMapEpochRecord>::value >= 8 &&
        std::alignment_of<Block>::value >= 8 &&
        std::alignment_of<LockFreeHashMapValueCacheEntry>::value >= 8 &&
        std::alignment_of<LockFreeHashMapPackedValueCacheEntry>::value >= 8, "atomic owners require eight byte alignment");
    Map stack;
    Map array[3];
    LockFreeHashMapAlignmentFixture embedded;
    std::unique_ptr<Map[]> heap(new (std::nothrow) Map[3]);
    check(heap.get() != nullptr, "alignment heap fixture allocated");
    Map* maps[] = { &stack, &array[0], &array[1], &array[2], &embedded.m_maps[0],
        &embedded.m_maps[1], &heap[0], &heap[1], &heap[2] };
    for (size_t index = 0; index < sizeof(maps) / sizeof(maps[0]); ++index)
    {
        Map& map = *maps[index];
        check(map.status() == LockFreeHashMapOk, "alignment map initialized");
        aligned(&LockFreeHashMapTestState::generation(map), "map generation");
        aligned(&LockFreeHashMapTestState::sequence(map), "map sequence");
        LockFreeHashMapEpoch& domain = *LockFreeHashMapTestState::storage(map).epoch();
        aligned(&LockFreeHashMapTestState::clock(domain), "allocated domain clock");
        for (size_t recordIndex = 0; recordIndex < domain.count(); ++recordIndex)
        {
            LockFreeHashMapEpochRecord* record = domain.record(recordIndex);
            check(record != nullptr, "alignment record exists");
            aligned(&record->m_epoch, "allocated record epoch");
            aligned(&record->m_countGeneration, "allocated record generation");
            aligned(&record->m_countDelta, "allocated record count");
        }
        check(map.set(7, 11), "aligned map publishes");
        map.clear();
        check(!map.contains(7), "aligned map clears");
        check(map.maintain(1024).m_status == LockFreeHashMapOk, "aligned map completes cleanup");
    }
    LockFreeHashMapEpoch domain;
    LockFreeHashMapEpochRecord records[3];
    aligned(&LockFreeHashMapTestState::clock(domain), "stack domain clock");
    for (size_t index = 0; index < 3; ++index)
    {
        aligned(&records[index].m_epoch, "stack record epoch");
        aligned(&records[index].m_countGeneration, "stack record generation");
        aligned(&records[index].m_countDelta, "stack record count");
    }
    Block block;
    aligned(&block.m_leases, "stack block leases");
    aligned(&block.m_retired, "stack block retired");
    // 只读目录末尾与借用计数至少间隔一条常见缓存行，不能依赖分配器起始地址
    const uintptr_t metadataEnd = reinterpret_cast<uintptr_t>(&block.m_words) + sizeof(block.m_words) - 1;
    const uintptr_t leasesBegin = reinterpret_cast<uintptr_t>(&block.m_leases);
    check(leasesBegin > metadataEnd && leasesBegin - metadataEnd >= 64,
        "block metadata is isolated from lease writes");
    check(block.initialize(193), "alignment block initialized");
    for (size_t index = 0; index < block.m_words * 2; ++index)
    {
        aligned(&block.m_bits[index], "allocated bitmap word");
    }
    for (size_t index = 0; index < block.m_count; ++index)
    {
        aligned(&block.m_slots[index].m_retired, "allocated slot retired");
    }
    LockFreeHashMapTestState::ScalarCache cache;
    LockFreeHashMapPackedValueCacheEntry packedStack[3];
    std::unique_ptr<LockFreeHashMapPackedValueCacheEntry[]> packedHeap(new (std::nothrow) LockFreeHashMapPackedValueCacheEntry[3]);
    check(packedHeap.get() != nullptr, "compact cache alignment fixture allocated");
    for (size_t index = 0; index < 3; ++index)
    {
        aligned(&packedStack[index].m_stamp, "stack compact stamp");
        aligned(&packedStack[index].m_generation, "stack compact generation");
        aligned(&packedStack[index].m_key, "stack compact payload");
        aligned(&packedHeap[index].m_stamp, "heap compact stamp");
        aligned(&packedHeap[index].m_generation, "heap compact generation");
        aligned(&packedHeap[index].m_key, "heap compact payload");
    }
    check(cache.initialize(8) == LockFreeHashMapOk, "alignment cache initialized");
    LockFreeHashMapValueCacheEntry* entries = &LockFreeHashMapTestState::first(cache);
    check((reinterpret_cast<uintptr_t>(entries) & 63) == 0, "wide cache allocation begins on a cache line");
    for (size_t index = 0; index < cache.count(); ++index)
    {
        aligned(&entries[index].m_stamp, "allocated cache stamp");
        aligned(&entries[index].m_generation, "allocated cache generation");
        aligned(&entries[index].m_key, "allocated cache key");
        aligned(&entries[index].m_value, "allocated cache value");
    }
    LockFreeHashMapTestState::PackedCache packed;
    check(packed.initialize(8) == LockFreeHashMapOk, "compact alignment cache initialized");
    LockFreeHashMapPackedValueCacheEntry* narrow = &LockFreeHashMapTestState::first(packed);
    check((reinterpret_cast<uintptr_t>(narrow) & 63) == 0, "compact cache allocation begins on a cache line");
    for (size_t index = 0; index < packed.count(); ++index)
    {
        uintptr_t begin = reinterpret_cast<uintptr_t>(narrow + index);
        uintptr_t end = begin + sizeof(LockFreeHashMapPackedValueCacheEntry) - 1;
        check((begin >> 6) == (end >> 6), "each compact cache entry stays within one cache line");
    }
    std::cout << "ALIGNMENT_RESULT maps=9 records=" << LockFreeHashMapTestState::storage(stack).epoch()->count() * 9 + 3 <<
        " slots=" << block.m_count << " bitmap_words=" << block.m_words * 2 << " cache_entries=" << cache.count() << " passed=1" << std::endl;
}

int LockFreeHashMapRewriteTests::run(bool alignmentOnly)
{
    try
    {
        if (alignmentOnly)
        {
            alignment();
            std::cout << "MODULE_RESULT checks=" << s_checks.load() << " failed=" << s_failed.load() << std::endl;
            return 0;
        }
        if (LockFreeHashMapPoolTests::run() != 0)
        {
            return 1;
        }
        int64_t allocations = LockFreeHashMapPoolTestAllocation::s_live.load();
        alignment();
        addressableValues();
        sharedExports();
        model();
        versions();
        exceptions();
        identityCaches();
        stringValues();
        scalarCache();
        largeTableLifecycle();
#ifndef LOCKFREEHASHMAP_TEST_SINGLE_THREAD
        paused();
        contention();
        collisions();
        scalarStress();
        narrowStress();
#endif
        check(LockFreeHashMapRewriteValue::s_live.load() == 0 && LockFreeHashMapTestThread::failures() == 0, "all values and worker responsibilities released");
        check(LockFreeHashMapPoolTestAllocation::s_live.load() == allocations, "rewrite suite allocation delta is zero");
        std::cout << "REWRITE_ALLOCATION_DELTA " << LockFreeHashMapPoolTestAllocation::s_live.load() - allocations << std::endl;
    }
    catch (...)
    {
        LockFreeHashMapPoolTestAllocation::s_fail.store(-1);
        LockFreeHashMapPoolTestAllocation::s_failSize.store(0);
        LockFreeHashMapRewriteValue::s_throw.store(false);
        LockFreeHashMapRewriteValue::s_resume.store(true);
        s_failed.fetch_add(1);
        // 故障注入已解除，补充异常类型，不能只输出不明原因的failed=1。
        try
        {
            throw;
        }
        catch (const std::exception& error)
        {
            std::cerr << "REWRITE_EXCEPTION " << error.what() << std::endl;
        }
        catch (int code)
        {
            std::cerr << "REWRITE_EXCEPTION code=" << code << std::endl;
        }
        catch (...)
        {
            std::cerr << "REWRITE_EXCEPTION unknown" << std::endl;
        }
    }
    std::cout << "MODULE_RESULT checks=" << s_checks.load() << " failed=" << s_failed.load() << std::endl;
    return s_failed.load() == 0 ? 0 : 1;
}

/** 独立模块入口，保留 Wasm 编排传入的模块编号，另提供对齐专项入口
@param [in] argc 参数数量
@param [in] argv 可省略参数，或传入 1 运行全模块，alignment 只运行对齐回归
@return 零通过，非零为检查失败或参数错误
*/
int main(int argc, char** argv)
{
    bool alignmentOnly = argc == 2 && std::strcmp(argv[1], "alignment") == 0;
    if (argc == 1 || alignmentOnly || (argc == 2 && std::strcmp(argv[1], "1") == 0))
    {
        return LockFreeHashMapRewriteTests::run(alignmentOnly);
    }
    return 2;
}