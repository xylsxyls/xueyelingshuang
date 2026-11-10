#pragma once
#include "LockFreeHashMapTests.h"

/** 测试模块独占的白盒访问，操作真实对象，调用者负责静止期或定向交错协议
不生成产品头副本，不使用偏移猜测、对象重解释或 private 宏替换
*/
class LockFreeHashMapTestState
{
public:
    typedef LockFreeHashMap<int, int> IntMap;
    typedef LockFreeHashMap<int, int, LockFreeHashMapCollisionHash> CollisionMap;
    typedef LockFreeHashMapValueCache<uint64_t, uint64_t, std::equal_to<uint64_t>> ScalarCache;
    typedef std::atomic<uint64_t> IntMap::* IntCounter;
    typedef std::atomic<uint64_t> CollisionMap::* CollisionCounter;
    typedef IntMap::Storage* IntMap::* StoragePointer;
    typedef std::atomic<bool> IntMap::* ValidPointer;
    typedef void (IntMap::* AccountPointer)(LockFreeHashMapEpochRecord& record, uint64_t generation, int64_t difference);
    typedef std::atomic<uint64_t> LockFreeHashMapEpoch::* ClockPointer;
    typedef LockFreeHashMapValueCacheEntry* ScalarCache::* EntriesPointer;

    /** 借用表的版本高水位，仅静止期注入
    @param [in,out] map 有效被测表
    @return 真实版本原子字段
    */
    static std::atomic<uint64_t>& sequence(IntMap& map);

    /** 借用全碰撞表的版本高水位，仅空表静止期注入
    @param [in,out] map 有效被测表
    @return 真实版本原子字段
    */
    static std::atomic<uint64_t>& historySequence(CollisionMap& map);

    /** 借用清空代次，耗尽用例在静止期注入
    @param [in,out] map 有效被测表
    @return 真实清空代次
    */
    static std::atomic<uint64_t>& generation(IntMap& map);

    /** 借用现有池，不转移所有权，不申请替身资源
    @param [in,out] map 已初始化的被测表
    @return 真实池对象
    */
    static IntMap::Storage& storage(IntMap& map);

    /** 借用分片计数有效标志，核对溢出后的回退
    @param [in,out] map 有效被测表
    @return 真实计数有效标志
    */
    static std::atomic<bool>& countValid(IntMap& map);

    /** 调用真实记账函数，测试极端计数及迟到旧代记账
    @param [in,out] map 有效且外部静止的表
    @param [in,out] record 属于该表的独占记录
    @param [in] generation 要模拟的记账代次
    @param [in] difference 条目数量差
    */
    static void account(IntMap& map, LockFreeHashMapEpochRecord& record, uint64_t generation, int64_t difference);

    /** 借用保护域时钟，仅无活动保护时注入
    @param [in,out] domain 有效保护域
    @return 真实保护域时钟
    */
    static std::atomic<uint64_t>& clock(LockFreeHashMapEpoch& domain);

    /** 借用第一个真实缓存格，夹具保证初始化成功及定向交错协议
    @param [in,out] cache 至少拥有一个缓存格的缓存
    @return 真实缓存格，不延长其寿命
    */
    static LockFreeHashMapValueCacheEntry& first(ScalarCache& cache);
private:
    /** 检查登记和夹具前提，失败抛测试状态交由现有入口记录
    @param [in] condition 前提满足为 true
    */
    static void require(bool condition);
private:
    // 静态字段为常量初始化，随后由同翻译单元的绑定对象登记，不在测试运行中修改
    static IntCounter s_sequence;
    // 清空代次成员指针
    static IntCounter s_generation;
    // 全碰撞实例的版本成员指针
    static CollisionCounter s_historySequence;
    // 真实池的成员指针
    static StoragePointer s_storage;
    // 计数有效标志的成员指针
    static ValidPointer s_countValid;
    // 真实记账函数的成员指针
    static AccountPointer s_account;
    // 保护域时钟的成员指针
    static ClockPointer s_clock;
    // 缓存数组的成员指针
    static EntriesPointer s_entries;
};