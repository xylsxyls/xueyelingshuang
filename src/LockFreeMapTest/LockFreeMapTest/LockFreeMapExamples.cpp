#include "LockFreeMap/LockFreeMapAPI.h"
#include <memory>
#include <type_traits>

/** 进程缓存淘汰和有序桶退役的接口编译及执行样例
@return 样例执行通过时返回零
*/
int LockFreeMapCompileExamples()
{
    // 每个目标编译完全相同的 C++ 接口合同
    // 不得替换为 Wasm 专用 C 句柄或包装接口
    typedef LockFreeMap<int, int> Map;
    typedef LockFreeMapStatus (Map::*Configure)(const LockFreeMapConfig&);
    typedef LockFreeMapStatus (Map::*Get)(const int&, int&, LockFreeMapToken&) const;
    typedef void (*Borrow)(const int&);
    typedef LockFreeMapStatus (Map::*Visit)(const int&, const Borrow&) const;
    typedef LockFreeMapStatus (Map::*GetOrInsert)(const int&, const int&, int&, LockFreeMapToken&);
    typedef LockFreeMapStatus (Map::*Update)(const int&, const LockFreeMapToken&, const int&);
    typedef LockFreeMapStatus (Map::*Erase)(const int&, const LockFreeMapToken&);
    typedef LockFreeMapStatus (Map::*Extract)(const int&, int&);
    typedef LockFreeMapStatus (Map::*Last)(int&, int&, LockFreeMapToken&) const;
    typedef LockFreeMapStatus (Map::*ExtractLast)(const LockFreeMapToken&, int&, int&);
    typedef LockFreeMapStatus (Map::*Set)(const int&, const int&);
    typedef LockFreeMapStatus (Map::*Maintain)(size_t, size_t&);
    typedef LockFreeMapStats (Map::*Stats)() const;
    typedef bool (Map::*Atomics)() const;
    static_assert(std::is_same<decltype(&Map::lockFreeMapConfigure), Configure>::value, "common configure signature");
    static_assert(std::is_same<decltype(&Map::lockFreeMapGet), Get>::value, "common get signature");
    static_assert(std::is_same<decltype(static_cast<Visit>(&Map::lockFreeMapVisit<Borrow>)), Visit>::value, "common borrowed visit signature");
    static_assert(std::is_same<decltype(&Map::lockFreeMapGetOrInsert), GetOrInsert>::value, "common get-or-insert signature");
    static_assert(std::is_same<decltype(&Map::lockFreeMapUpdate), Update>::value, "common update signature");
    static_assert(std::is_same<decltype(&Map::lockFreeMapErase), Erase>::value, "common erase signature");
    static_assert(std::is_same<decltype(&Map::lockFreeMapExtract), Extract>::value, "common extract signature");
    static_assert(std::is_same<decltype(&Map::lockFreeMapLast), Last>::value, "common last signature");
    static_assert(std::is_same<decltype(&Map::lockFreeMapExtractLast), ExtractLast>::value, "common conditional maximum signature");
    static_assert(std::is_same<decltype(&Map::lockFreeMapSet), Set>::value, "common set signature");
    static_assert(std::is_same<decltype(&Map::lockFreeMapMaintain), Maintain>::value, "common maintain signature");
    static_assert(std::is_same<decltype(&Map::lockFreeMapStats), Stats>::value, "common stats signature");
    static_assert(std::is_same<decltype(&Map::lockFreeMapAtomicsSupported), Atomics>::value, "common atomic capability signature");
    typedef bool (Map::*LegacyWrite)(const int&, const int&);
    typedef bool (Map::*LegacyErase)(const int&);
    typedef bool (Map::*LegacyRead)(const int&, int&) const;
    typedef bool (Map::*Contains)(const int&) const;
    typedef bool (Map::*Edge)(int&, int&) const;
    typedef bool (Map::*Bound)(const int&, int&, int&) const;
    typedef size_t (Map::*CountKey)(const int&) const;
    typedef size_t (Map::*Count)() const;
    typedef void (Map::*Clear)();
    typedef int (Map::*Value)(const int&, const int&) const;
    typedef std::vector<int> (Map::*Vector)() const;
    typedef std::vector<std::pair<int, int>> (Map::*Pairs)() const;
    typedef void (*Callback)(const int&, const int&);
    typedef size_t (Map::*ForEach)(Callback) const;
    static_assert(std::is_same<decltype(&Map::set), LegacyWrite>::value, "common legacy set");
    static_assert(std::is_same<decltype(&Map::insert), LegacyWrite>::value, "common legacy insert");
    static_assert(std::is_same<decltype(&Map::erase), LegacyErase>::value, "common legacy erase");
    static_assert(std::is_same<decltype(&Map::remove), LegacyErase>::value, "common legacy remove");
    static_assert(std::is_same<decltype(&Map::get), LegacyRead>::value, "common legacy get");
    static_assert(std::is_same<decltype(&Map::contains), Contains>::value, "common contains");
    static_assert(std::is_same<decltype(&Map::first), Edge>::value, "common first");
    static_assert(std::is_same<decltype(&Map::last), Edge>::value, "common legacy last");
    static_assert(std::is_same<decltype(&Map::lowerBound), Bound>::value, "common lower bound");
    static_assert(std::is_same<decltype(&Map::upperBound), Bound>::value, "common upper bound");
    static_assert(std::is_same<decltype(static_cast<CountKey>(&Map::count)), CountKey>::value, "common key count");
    static_assert(std::is_same<decltype(static_cast<Count>(&Map::count)), Count>::value, "common count");
    static_assert(std::is_same<decltype(&Map::size), Count>::value, "common size");
    static_assert(std::is_same<decltype(&Map::maxLevel), Count>::value, "common maxLevel");
    static_assert(std::is_same<decltype(&Map::retainedNodeCount), Count>::value, "common retained keys");
    static_assert(std::is_same<decltype(&Map::retainedValueCount), Count>::value, "common retained values");
    static_assert(std::is_same<decltype(&Map::empty), Atomics>::value, "common empty");
    static_assert(std::is_same<decltype(&Map::clear), Clear>::value, "common clear");
    static_assert(std::is_same<decltype(&Map::clearAndFreeMemory), Clear>::value, "common free");
    static_assert(std::is_same<decltype(&Map::value), Value>::value, "common value");
    static_assert(std::is_same<decltype(&Map::keys), Vector>::value, "common keys");
    static_assert(std::is_same<decltype(&Map::values), Vector>::value, "common values");
    static_assert(std::is_same<decltype(&Map::toVector), Pairs>::value, "common pairs");
    static_assert(std::is_same<decltype(static_cast<ForEach>(&Map::forEach<Callback>)), ForEach>::value, "common traversal");
    static_assert(std::is_same<decltype(&Map::operator[]), MapValueProxy<int, int, std::less<int>, 16> (Map::*)(const int&)>::value,
        "common subscript proxy");

    LockFreeMap<int, std::shared_ptr<int>> cache;
    LockFreeMapConfig config;
    config.nodes.maximumCapacity = 1024;
    config.values.maximumCapacity = 2048;
    config.nodes.grow = [](const LockFreeMapGrowContext&) -> size_t
    {
        return 32;
    };
    config.values.grow = [](const LockFreeMapGrowContext& context) -> size_t
    {
        return context.currentCapacity / 2 + 1;
    };
    if (cache.lockFreeMapConfigure(config) != LOCK_FREE_MAP_OK)
    {
        return 1;
    }
    std::shared_ptr<int> candidate(new int(7));
    std::shared_ptr<int> channel;
    LockFreeMapToken identity;
    LockFreeMapStatus status = cache.lockFreeMapGetOrInsert(123, candidate, channel, identity);
    if (status != LOCK_FREE_MAP_COMMITTED && status != LOCK_FREE_MAP_EXISTS)
    {
        return 2;
    }
    const long references = candidate.use_count();
    unsigned visits = 0;
    bool borrowedValid = false;
    // 借用不增加载荷 shared_ptr 的拥有计数，六端均直接使用同一模板接口
    status = cache.lockFreeMapVisit(123, [&candidate, &visits, &borrowedValid, references](const std::shared_ptr<int>& borrowed)
    {
        ++visits;
        borrowedValid = borrowed.get() == candidate.get() && borrowed.use_count() == references && *borrowed == 7;
    });
    if (status != LOCK_FREE_MAP_OK || visits != 1 || !borrowedValid)
    {
        return 6;
    }

    Map borrowedMap;
    if (!borrowedMap.set(5, 7))
    {
        return 7;
    }
    visits = 0;
    borrowedValid = false;
    // 回调重入清空和重插后，当前借用仍指向原来的不可变版本
    status = borrowedMap.lockFreeMapVisit(5, [&borrowedMap, &visits, &borrowedValid](const int& borrowed)
    {
        ++visits;
        borrowedMap.clear();
        size_t released = 0;
        borrowedValid = borrowedMap.lockFreeMapMaintain(8, released) == LOCK_FREE_MAP_OK &&
            borrowedMap.set(5, 9) && borrowed == 7;
    });
    if (status != LOCK_FREE_MAP_OK || visits != 1 || !borrowedValid)
    {
        return 8;
    }
    // 找不到键不得执行回调；用户回调异常由接口转为状态，保护记录必须归还
    visits = 0;
    const auto throwing = [&visits](const int&)
    {
        ++visits;
        throw 7;
    };
    if (borrowedMap.lockFreeMapVisit(6, throwing) != LOCK_FREE_MAP_NOT_FOUND || visits != 0 ||
        borrowedMap.lockFreeMapVisit(5, throwing) != LOCK_FREE_MAP_USER_EXCEPTION || visits != 1)
    {
        return 9;
    }
    int borrowedValue = 0;
    if (!borrowedMap.get(5, borrowedValue) || borrowedValue != 9)
    {
        return 10;
    }
    // 退役缓存引用不能关闭正在使用的通道
    status = cache.lockFreeMapErase(123, identity);
    if (status != LOCK_FREE_MAP_COMMITTED || !channel || *channel != 7)
    {
        return 3;
    }

    LockFreeMap<int, std::shared_ptr<int>> priorities;
    priorities.set(1, channel);
    priorities.set(2, candidate);
    int priority = 0;
    LockFreeMapToken selection;
    if (priorities.lockFreeMapLast(priority, channel, selection) != LOCK_FREE_MAP_OK)
    {
        return 4;
    }
    // 桶退役前须独立关闭准入并等待在途生产者结束
    // 移除索引并不同时原子选中桶内的某个任务
    status = priorities.lockFreeMapExtractLast(selection, priority, channel);
    return status == LOCK_FREE_MAP_COMMITTED && priority == 2 ? 0 : 5;
}