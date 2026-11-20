#include "LockFreeMapStatusTest.h"
#include "LockFreeMapTest.h"
#include "LockFreeMap/LockFreeMap.h"
#include <stdexcept>
#include <new>
#include <limits>

LockFreeMapPolicyCopyFault::LockFreeMapPolicyCopyFault(bool allocation) :
m_allocation(allocation)
{

}

LockFreeMapPolicyCopyFault::LockFreeMapPolicyCopyFault(const LockFreeMapPolicyCopyFault& other) :
m_allocation(other.m_allocation)
{
    if (m_allocation)
    {
        throw std::bad_alloc();
    }
    throw std::runtime_error("policy copy");
}

std::size_t LockFreeMapPolicyCopyFault::operator()(const LockFreeMapGrowContext& context) const
{
    (void)context;
    return 0;
}

LockFreeMapShrinkDecision LockFreeMapPolicyCopyFault::operator()(const LockFreeMapShrinkContext& context) const
{
    (void)context;
    return LockFreeMapShrinkDecision();
}

void LockFreeMapPolicyCopyFault::operator()(std::size_t added, uint64_t count) const
{
    (void)added;
    (void)count;
}

LockFreeMapStatusKey::LockFreeMapStatusKey(int value, std::function<void()>* compare) :
m_value(value),
m_compare(compare)
{

}

LockFreeMapStatusKey::LockFreeMapStatusKey(const LockFreeMapStatusKey& other) :
m_value(other.m_value),
m_compare(nullptr)
{

}

LockFreeMapStatusKey& LockFreeMapStatusKey::operator=(const LockFreeMapStatusKey& other)
{
    m_value = other.m_value;
    m_compare = nullptr;
    return *this;
}

bool LockFreeMapStatusKey::operator<(const LockFreeMapStatusKey& other) const
{
    std::function<void()>* action = m_compare != nullptr ? m_compare : other.m_compare;
    if (action != nullptr)
    {
        (*action)();
    }
    return m_value < other.m_value;
}

LockFreeMapStatusValue::LockFreeMapStatusValue(int value, std::function<void()>* assign) :
m_value(value),
m_assign(assign)
{

}

LockFreeMapStatusValue::LockFreeMapStatusValue(const LockFreeMapStatusValue& other) :
m_value(other.m_value),
m_assign(nullptr)
{

}

LockFreeMapStatusValue& LockFreeMapStatusValue::operator=(const LockFreeMapStatusValue& other)
{
    if (m_assign != nullptr)
    {
        (*m_assign)();
    }
    m_value = other.m_value;
    return *this;
}

bool LockFreeMapStatusValue::operator<(const LockFreeMapStatusValue& other) const
{
    return m_value < other.m_value;
}

/** 使输出与查询键或候选重叠，检查发布顺序和重试输入
@return 两种公开入口都保留原输入且未删错键时true
*/
static bool CheckOverlappingArguments()
{
    for (int mode = 0; mode != 2; ++mode)
    {
        LockFreeMap<int, int> map;
        int key = 2;
        LockFreeMapToken token;
        if (!map.set(5, 50))
        {
            return false;
        }
        const LockFreeMapStatus result = mode == 0 ? map.lockFreeMapGetOrInsert(key, 7, key, token) :
            map.tryGetOrInsert(key, 7, key, token, true, false);
        if (result != LOCK_FREE_MAP_COMMITTED || key != 7 || map.contains(7) || map.value(2) != 7 ||
            map.value(5) != 50 || map.lockFreeMapUpdate(2, token, 8) != LOCK_FREE_MAP_COMMITTED)
        {
            return false;
        }
        LockFreeMap<int, LockFreeMapStatusValue> nested;
        if (!nested.set(5, LockFreeMapStatusValue(7)) || !nested.set(7, LockFreeMapStatusValue(99)))
        {
            return false;
        }
        unsigned calls = 0;
        bool changed = false;
        /** 首次复制输出时修改无关键，迫使外层提取重新验证根
        */
        std::function<void()> churn = [&]()
        {
            if (++calls == 1)
            {
                changed = nested.set(1, LockFreeMapStatusValue(1));
            }
        };
        LockFreeMapStatusValue output(5, &churn);
        const LockFreeMapStatus taken = mode == 0 ? nested.lockFreeMapExtract(output.m_value, output) :
            nested.tryExtract(output.m_value, output, true, false);
        if (taken != LOCK_FREE_MAP_COMMITTED || calls != 2 || !changed || output.m_value != 7 ||
            nested.contains(5) || !nested.contains(7))
        {
            return false;
        }
        calls = 0;
        /** 写输出时改变候选原对象，仍须提交调用开始时的候选
        */
        churn = [&]()
        {
            if (++calls == 1)
            {
                output.m_value = 123;
                changed = nested.erase(1);
            }
        };
        const LockFreeMapStatus created = mode == 0 ? nested.lockFreeMapGetOrInsert(3, output, output, token) :
            nested.tryGetOrInsert(3, output, output, token, true, false);
        if (created != LOCK_FREE_MAP_COMMITTED || calls != 2 || !changed || output.m_value != 7 ||
            nested.value(3).m_value != 7)
        {
            return false;
        }
        LockFreeMap<LockFreeMapStatusValue, LockFreeMapToken> priorities;
        const LockFreeMapToken payload;
        LockFreeMapToken out;
        LockFreeMapStatusValue selected;
        if (!priorities.set(LockFreeMapStatusValue(4), payload) ||
            priorities.lockFreeMapLast(selected, out, token) != LOCK_FREE_MAP_OK)
        {
            return false;
        }
        out = token;
        calls = 0;
        /** 键输出第一次赋值时插入较低优先级，随后值输出覆盖原凭据
        */
        churn = [&]()
        {
            if (++calls == 1)
            {
                changed = priorities.set(LockFreeMapStatusValue(1), payload);
            }
        };
        selected.m_assign = &churn;
        const LockFreeMapStatus last = mode == 0 ? priorities.lockFreeMapExtractLast(out, selected, out) :
            priorities.tryExtractLast(out, selected, out, true, false);
        if (last != LOCK_FREE_MAP_COMMITTED || calls != 2 || !changed || selected.m_value != 4 ||
            out.m_identity || out.m_version != 0 || priorities.size() != 1 || !priorities.contains(LockFreeMapStatusValue(1)))
        {
            return false;
        }
    }
    return true;
}

/** 比较清空后首次写入使用不同自动维护预算的实际回收量
@param [in] budget 待验证的正预算
@param [out] retained 成功时仍然存活的键对象数
@return 配置、插入和清空后的新键提交都成功时true
*/
static bool ObserveMaintenance(size_t budget, size_t& retained)
{
    LockFreeMapConfig config;
    LockFreeMap<int, int> map(false);
    if (config.setMaintenanceBudget(budget) != LOCK_FREE_MAP_OK ||
        map.lockFreeMapConfigure(config) != LOCK_FREE_MAP_OK)
    {
        return false;
    }
    for (int key = 0; key != 12; ++key)
    {
        if (!map.insert(key, key))
        {
            return false;
        }
    }
    map.clear();
    if (!map.insert(31, 45))
    {
        return false;
    }
    retained = map.retainedNodeCount();
    return map.size() == 1 && map.value(31) == 45;
}

/** 验证单个操作记录在指定写入次数时触发维护，覆盖之间不发生缺槽或结构变化
@param [in] interval 待设置的正间隔
@param [in] lastAttempt 含首次插入在内的写入次数
@return 每次覆盖后的值与维护回调次数均符合独立预期时为true
*/
static bool CheckMaintenanceInterval(uint32_t interval, unsigned lastAttempt)
{
    LockFreeMapConfig config;
    unsigned calls = 0;
    /** 只记录键池维护，不要求释放块
    @note 未命名参数为键池容量观察
    @return 不缩容
    */
    const auto observe = [&calls](const LockFreeMapShrinkContext&)
    {
        ++calls;
        return LockFreeMapShrinkDecision();
    };
    LockFreeMap<int, int> map(false);
    if (config.setMaintenanceInterval(interval) != LOCK_FREE_MAP_OK ||
        config.setMaintenanceInterval(0) != LOCK_FREE_MAP_INVALID ||
        config.nodes.setShrink(observe) != LOCK_FREE_MAP_OK ||
        map.lockFreeMapConfigure(config) != LOCK_FREE_MAP_OK || !map.insert(9, 1))
    {
        return false;
    }
    // 首次插入空表会触发结构维护，也占一次写入计数。
    unsigned expected = calls;
    for (unsigned attempt = 2; attempt <= lastAttempt; ++attempt)
    {
        const int value = static_cast<int>(attempt);
        if (attempt % interval == 0)
        {
            ++expected;
        }
        if (!map.set(9, value) || map.value(9) != value || calls != expected)
        {
            return false;
        }
    }
    return true;
}

/** 检查双池零容量起步的默认增长边界及清空后的槽复用
@return 两池仅在缺槽时从零增至16再增至32，清空与维护后可复用时返回true
*/
static bool CheckDefaultPoolGrowth()
{
    LockFreeMapConfig config;
    LockFreeMap<int, int> map(false);
    if (config.nodes.setCapacity(0, -1) != LOCK_FREE_MAP_OK || config.values.setCapacity(0, -1) != LOCK_FREE_MAP_OK ||
        map.lockFreeMapConfigure(config) != LOCK_FREE_MAP_OK)
    {
        return false;
    }
    const LockFreeMapStats initial = map.lockFreeMapStats();
    if (initial.nodes.capacity != 0 || initial.values.capacity != 0)
    {
        return false;
    }
    for (int key = 0; key <= 16; ++key)
    {
        if (!map.insert(key, key + 10))
        {
            return false;
        }
        const size_t expected = key < 16 ? 16 : 32;
        const uint64_t grows = key < 16 ? 1 : 2;
        const LockFreeMapStats stats = map.lockFreeMapStats();
        if (stats.nodes.capacity != expected || stats.values.capacity != expected ||
            stats.nodes.successfulGrowCount != grows || stats.values.successfulGrowCount != grows)
        {
            return false;
        }
    }
    map.clear();
    size_t released = 0;
    if (map.lockFreeMapMaintain(512, released) != LOCK_FREE_MAP_OK || released != 0 || !map.empty() ||
        !map.insert(23, 37) || map.value(23) != 37)
    {
        return false;
    }
    const LockFreeMapStats reused = map.lockFreeMapStats();
    return reused.nodes.capacity == 32 && reused.values.capacity == 32 &&
        reused.nodes.successfulGrowCount == 2 && reused.values.successfulGrowCount == 2;
}

bool LockFreeMapConfigurationTest()
{
    if (!CheckDefaultPoolGrowth())
    {
        return false;
    }
    LockFreeMapConfig config;
    LockFreeMap<int, int> defaults;
    const LockFreeMapStats initial = defaults.lockFreeMapStats();
    if (initial.nodes.capacity != 16 || initial.values.capacity != 16 || config.operationRecordLimit != 64 ||
        config.maintenanceBudget != 64 || config.maintenanceInterval != 64 ||
        config.nodes.maximumCapacity != -1 || config.nodes.blockRecordLimit != 256 ||
        config.nodes.growFreeThreshold != 0 || config.nodes.m_onGrowth)
    {
        return false;
    }
    size_t growth = 0, shrinks = 0;
    /** 每批新增三个槽，拒绝不符合预期的上下文
    @param [in] context 增长前的初始及当前容量
    @return 初始容量符合测试配置时为三
    */
    const auto grow = [](const LockFreeMapGrowContext& context) -> size_t
    {
        return context.initialCapacity == 2 && context.currentCapacity >= 2 ? 3 : 0;
    };
    /** 统计实际发布数，不计初始容量及失败增长
    @param [in] added 本次实际新增槽数
    @param [in] count 已成功增长的次数
    */
    const auto notify = [&](size_t added, uint64_t count)
    {
        growth += count == 1 ? added : 1000;
    };
    /** 记录缩容回调的调用，保持容量以检查预增长
    @param [in] context 当前容量观察
    @return 禁用释放的决策
    */
    const auto shrink = [&](const LockFreeMapShrinkContext& context)
    {
        shrinks += context.initialCapacity == 2 ? 1 : 0;
        return LockFreeMapShrinkDecision();
    };
    config.nodes.setGrowFreeThreshold(1);
    if (config.nodes.setCapacity(2, 4) != LOCK_FREE_MAP_OK || config.nodes.setCapacity(3, 1) != LOCK_FREE_MAP_INVALID ||
        config.nodes.setCapacity(0, -2) != LOCK_FREE_MAP_INVALID || config.nodes.setBlockRecordLimit(2) != LOCK_FREE_MAP_OK ||
        config.nodes.setBlockRecordLimit(0) != LOCK_FREE_MAP_INVALID || config.nodes.setGrow(grow) != LOCK_FREE_MAP_OK ||
        config.nodes.setShrink(shrink) != LOCK_FREE_MAP_OK || config.nodes.setOnGrowth(notify) != LOCK_FREE_MAP_OK ||
        config.values.setCapacity(8, -1) != LOCK_FREE_MAP_OK || config.setOperationRecordLimit(1) != LOCK_FREE_MAP_OK ||
        config.setOperationRecordLimit(0) != LOCK_FREE_MAP_INVALID || config.setMaintenanceBudget(0) != LOCK_FREE_MAP_INVALID)
    {
        return false;
    }
    LockFreeMap<int, int> map(false);
    if (map.lockFreeMapSet(1, 1) != LOCK_FREE_MAP_INVALID || map.lockFreeMapConfigure(config) != LOCK_FREE_MAP_OK ||
        map.lockFreeMapStats().nodes.capacity != 2 || map.lockFreeMapStats().values.capacity != 8 ||
        !map.insert(1, 1) || growth != 2 || shrinks == 0 || map.lockFreeMapStats().nodes.capacity != 4)
    {
        return false;
    }
    LockFreeMapStatus nested = LOCK_FREE_MAP_OK;
    /** 唯一记录已由访问回调占用，重入写入应明确报告记录不足
    @param [in] value 当前借用的值
    */
    const auto reenter = [&](const int& value) { nested = map.lockFreeMapSet(9, value); };
    if (map.lockFreeMapVisit(1, reenter) != LOCK_FREE_MAP_OK || nested != LOCK_FREE_MAP_RECORD_LIMIT ||
        !map.insert(2, 2) || !map.insert(3, 3) || !map.insert(4, 4) ||
        map.tryInsert(5, 5) != LOCK_FREE_MAP_CAPACITY || map.contains(5))
    {
        return false;
    }
    map.clearAndFreeMemory();
    config.nodes.setGrowFreeThreshold(0);
    if (config.nodes.setCapacity(2, -1) != LOCK_FREE_MAP_OK || config.nodes.setBlockRecordLimit(1) != LOCK_FREE_MAP_OK ||
        map.lockFreeMapConfigure(config) != LOCK_FREE_MAP_OK || !map.insert(1, 1) || !map.insert(2, 2) ||
        map.tryInsert(3, 3) != LOCK_FREE_MAP_RECORD_LIMIT)
    {
        return false;
    }
    map.clearAndFreeMemory();
    if (config.nodes.setCapacity(0, 0) != LOCK_FREE_MAP_OK ||
        config.nodes.setGrow(std::function<size_t(const LockFreeMapGrowContext&)>()) != LOCK_FREE_MAP_OK ||
        map.lockFreeMapConfigure(config) != LOCK_FREE_MAP_OK || map.trySet(1, 1) != LOCK_FREE_MAP_CAPACITY ||
        !map.empty())
    {
        return false;
    }
    // 只放宽上限，保持零初始容量和刚清除的增长策略，区分额度用完和策略拒绝。
    if (config.nodes.setCapacity(0, -1) != LOCK_FREE_MAP_OK || map.lockFreeMapConfigure(config) != LOCK_FREE_MAP_OK ||
        map.tryInsert(7, 9) != LOCK_FREE_MAP_GROW_DENIED || !map.empty() || map.lockFreeMapStats().nodes.capacity != 0)
    {
        return false;
    }
    // 独立配置值池。首次插入触发预增长，覆盖仍只替换值；两次操作都不应增长键池。
    LockFreeMapConfig valueConfig;
    growth = 0;
    shrinks = 0;
    valueConfig.values.setGrowFreeThreshold(1);
    if (valueConfig.values.setCapacity(2, 6) != LOCK_FREE_MAP_OK ||
        valueConfig.values.setBlockRecordLimit(2) != LOCK_FREE_MAP_OK ||
        valueConfig.values.setGrow(grow) != LOCK_FREE_MAP_OK ||
        valueConfig.values.setGrow(LockFreeMapPolicyCopyFault(true)) != LOCK_FREE_MAP_ALLOCATION_FAILURE ||
        valueConfig.values.setGrow(LockFreeMapPolicyCopyFault(false)) != LOCK_FREE_MAP_USER_EXCEPTION ||
        valueConfig.values.setShrink(shrink) != LOCK_FREE_MAP_OK ||
        valueConfig.values.setShrink(LockFreeMapPolicyCopyFault(true)) != LOCK_FREE_MAP_ALLOCATION_FAILURE ||
        valueConfig.values.setShrink(LockFreeMapPolicyCopyFault(false)) != LOCK_FREE_MAP_USER_EXCEPTION ||
        valueConfig.values.setOnGrowth(notify) != LOCK_FREE_MAP_OK ||
        valueConfig.values.setOnGrowth(LockFreeMapPolicyCopyFault(true)) != LOCK_FREE_MAP_ALLOCATION_FAILURE ||
        valueConfig.values.setOnGrowth(LockFreeMapPolicyCopyFault(false)) != LOCK_FREE_MAP_USER_EXCEPTION ||
        map.lockFreeMapConfigure(valueConfig) != LOCK_FREE_MAP_OK || !map.insert(1, 17) ||
        !map.set(1, 23) || map.value(1) != 23 || growth != 3 || shrinks == 0)
    {
        return false;
    }
    const LockFreeMapStats valueStats = map.lockFreeMapStats();
    if (valueStats.nodes.capacity != 16 || valueStats.nodes.successfulGrowCount != 0 ||
        valueStats.values.capacity != 5 || valueStats.values.successfulGrowCount != 1)
    {
        return false;
    }
    // 覆盖已有键也需要一个新值槽。分别只放宽容量和块目录，检查两种失败原因。
    LockFreeMapConfig limit;
    LockFreeMap<int, int> bounded(false);
    if (limit.values.setCapacity(1, 1) != LOCK_FREE_MAP_OK || limit.values.setBlockRecordLimit(1) != LOCK_FREE_MAP_OK ||
        bounded.lockFreeMapConfigure(limit) != LOCK_FREE_MAP_OK || !bounded.set(1, 10) ||
        bounded.trySet(1, 20) != LOCK_FREE_MAP_CAPACITY || bounded.value(1) != 10)
    {
        return false;
    }
    bounded.clearAndFreeMemory();
    if (limit.values.setCapacity(1, -1) != LOCK_FREE_MAP_OK || bounded.lockFreeMapConfigure(limit) != LOCK_FREE_MAP_OK ||
        !bounded.set(1, 10) || bounded.trySet(1, 20) != LOCK_FREE_MAP_RECORD_LIMIT || bounded.value(1) != 10)
    {
        return false;
    }
    bounded.clearAndFreeMemory();
    if (limit.values.setBlockRecordLimit(2) != LOCK_FREE_MAP_OK || bounded.lockFreeMapConfigure(limit) != LOCK_FREE_MAP_OK ||
        !bounded.set(1, 10) || bounded.trySet(1, 20) != LOCK_FREE_MAP_COMMITTED || bounded.value(1) != 20 ||
        bounded.lockFreeMapStats().values.capacity != 2)
    {
        return false;
    }
    size_t retainedSmall = 0, retainedLarge = 0;
    return ObserveMaintenance(1, retainedSmall) && ObserveMaintenance(64, retainedLarge) &&
        retainedSmall > retainedLarge && retainedLarge == 1 &&
        CheckMaintenanceInterval(1, 3) && CheckMaintenanceInterval(3, 7) && CheckMaintenanceInterval(64, 129) &&
        CheckMaintenanceInterval((std::numeric_limits<uint32_t>::max)(), 129);
}

int LockFreeMapStatusContract()
{
    try
    {
        if (!CheckOverlappingArguments())
        {
            return 28;
        }
        // 分别启用键池和值池的缩容路径，逐个核对成功写入归还唯一操作记录及最终载荷。
        // 单记录预算使遗漏归还不能被另一个空闲记录掩盖；此检查也在单线程 Wasm 执行。
        for (unsigned flags = 0; flags < 4; ++flags)
        {
            LockFreeMap<int, int> trial(false);
            LockFreeMapConfig settings;
            settings.operationRecordLimit = 1;
            settings.nodes.initialCapacity = settings.values.initialCapacity = 4;
            settings.nodes.maximumCapacity = settings.values.maximumCapacity = 64;
            settings.nodes.growFreeThreshold = settings.values.growFreeThreshold = 1;
            if ((flags & 1) != 0)
            {
                /** 为节点池启用短空闲缩容，覆盖自动回收路径
                @note 第1个未命名参数是同步借用输入，本回调不使用其内容
                @return 请求释放4槽并要求连续空闲1毫秒
                */
                settings.nodes.shrink = [](const LockFreeMapShrinkContext&)
                {
                    return LockFreeMapShrinkDecision(4, 1);
                };
            }
            if ((flags & 2) != 0)
            {
                /** 为值池启用短空闲缩容，覆盖自动回收路径
                @note 第1个未命名参数是同步借用输入，本回调不使用其内容
                @return 请求释放4槽并要求连续空闲1毫秒
                */
                settings.values.shrink = [](const LockFreeMapShrinkContext&)
                {
                    return LockFreeMapShrinkDecision(4, 1);
                };
            }
            if (trial.lockFreeMapConfigure(settings) != LOCK_FREE_MAP_OK)
            {
                return 20;
            }
            // 只观察公开统计，未在生产库插入任何检查或测试分支。
            /** 在串行用例中观察操作记录是否已归还
            @return activeOperations观察值为零时返回true，不代替生命周期同步
            */
            const auto idle = [&trial]() { return trial.lockFreeMapStats().activeOperations == 0; };
            for (unsigned iteration = 0; iteration < 32; ++iteration)
            {
                int result = 0, highest = 0;
                LockFreeMapToken current;
                if (!trial.set(1, 10) || !idle() ||
                    trial.lockFreeMapSet(1, 11) != LOCK_FREE_MAP_COMMITTED || !idle())
                {
                    return 21;
                }
                if (trial.lockFreeMapGetOrInsert(2, 20, result, current) != LOCK_FREE_MAP_COMMITTED ||
                    result != 20 || !idle() || trial.lockFreeMapUpdate(2, current, 21) != LOCK_FREE_MAP_COMMITTED || !idle())
                {
                    return 22;
                }
                if (trial.lockFreeMapGet(2, result, current) != LOCK_FREE_MAP_OK || result != 21 ||
                    trial.lockFreeMapErase(2, current) != LOCK_FREE_MAP_COMMITTED || !idle())
                {
                    return 23;
                }
                if (trial.lockFreeMapGetOrInsert(3, 30, result, current) != LOCK_FREE_MAP_COMMITTED || !idle() ||
                    trial.lockFreeMapExtract(3, result) != LOCK_FREE_MAP_COMMITTED || result != 30 || !idle())
                {
                    return 24;
                }
                if (!trial.set(4, 40) || !idle() || trial.lockFreeMapLast(highest, result, current) != LOCK_FREE_MAP_OK ||
                    highest != 4 || trial.lockFreeMapExtractLast(current, highest, result) != LOCK_FREE_MAP_COMMITTED ||
                    highest != 4 || result != 40 || !idle() || !trial.erase(1) || !idle() || !trial.empty())
                {
                    return 25;
                }
                size_t released = 0;
                if (trial.lockFreeMapMaintain(128, released) != LOCK_FREE_MAP_OK ||
                    trial.lockFreeMapMaintain(128, released) != LOCK_FREE_MAP_OK ||
                    trial.retainedNodeCount() != 0 || trial.retainedValueCount() != 0)
                {
                    return 26;
                }
            }
            trial.clearAndFreeMemory();
            if (!idle() || trial.retainedNodeCount() != 0 || trial.retainedValueCount() != 0)
            {
                return 27;
            }
        }
        LockFreeMap<LockFreeMapStatusKey, LockFreeMapStatusValue> map;
        const LockFreeMapStatusKey key(5), lower(1), other(100);
        unsigned changes = 0, errors = 0;
        // 只修改较小的无关键；最大项和值版本均不变，每轮重新发布一个根
        /** 至多256次修改较小无关键，保留最大项及其值版本
        */
        std::function<void()> churn = [&map, &changes, &errors, &lower]()
        {
            if (changes < 256)
            {
                ++changes;
                errors += map.set(lower, LockFreeMapStatusValue(1)) && map.erase(lower) ? 0 : 1;
            }
        };
        const LockFreeMapStatusKey query(5, &churn);
        LockFreeMapStatusValue value;
        LockFreeMapToken token;
        if (!map.set(key, LockFreeMapStatusValue(7)) ||
            map.lockFreeMapSet(query, LockFreeMapStatusValue(8)) != LOCK_FREE_MAP_COMMITTED || changes != 256)
        {
            return 1;
        }
        changes = 0;
        if (map.lockFreeMapGetOrInsert(query, LockFreeMapStatusValue(9), value, token) != LOCK_FREE_MAP_EXISTS ||
            changes != 256 || value.m_value != 8)
        {
            return 2;
        }
        changes = 0;
        if (map.lockFreeMapUpdate(query, token, LockFreeMapStatusValue(10)) != LOCK_FREE_MAP_COMMITTED ||
            changes != 256 || map.lockFreeMapUpdate(key, token, LockFreeMapStatusValue(11)) != LOCK_FREE_MAP_CONFLICT)
        {
            return 3;
        }
        if (map.lockFreeMapGet(key, value, token) != LOCK_FREE_MAP_OK || value.m_value != 10)
        {
            return 4;
        }
        changes = 0;
        if (map.lockFreeMapErase(query, token) != LOCK_FREE_MAP_COMMITTED || changes != 256 || !map.empty())
        {
            return 5;
        }
        // 插入尚不存在的键也必须重试；保留稳定锚点，确保每轮都触发比较交错
        if (!map.set(other, LockFreeMapStatusValue(100)))
        {
            return 6;
        }
        changes = 0;
        if (map.lockFreeMapGetOrInsert(query, LockFreeMapStatusValue(12), value, token) != LOCK_FREE_MAP_COMMITTED ||
            changes != 256 || value.m_value != 12 || !map.erase(other))
        {
            return 7;
        }
        changes = 0;
        if (map.lockFreeMapExtract(query, value) != LOCK_FREE_MAP_COMMITTED || changes != 256 ||
            value.m_value != 12 || map.lockFreeMapExtract(key, value) != LOCK_FREE_MAP_NOT_FOUND)
        {
            return 8;
        }
        if (!map.set(key, LockFreeMapStatusValue(13)) || map.lockFreeMapGet(key, value, token) != LOCK_FREE_MAP_OK)
        {
            return 9;
        }
        changes = 0;
        value.m_assign = &churn;
        LockFreeMapStatusKey extracted;
        if (map.lockFreeMapExtractLast(token, extracted, value) != LOCK_FREE_MAP_COMMITTED || changes != 256 ||
            extracted.m_value != 5 || value.m_value != 13 || !map.empty())
        {
            return 10;
        }
        value.m_assign = nullptr;
        // 输出复制期间出现更高键，条件已失效，不能刷新凭据后改取新条目
        if (!map.set(key, LockFreeMapStatusValue(14)) || map.lockFreeMapGet(key, value, token) != LOCK_FREE_MAP_OK)
        {
            return 11;
        }
        /** 在输出赋值阶段插入更高键，制造末项条件失效
        */
        std::function<void()> higher = [&map, &other, &errors]()
        {
            errors += map.set(other, LockFreeMapStatusValue(100)) ? 0 : 1;
        };
        value.m_assign = &higher;
        if (map.lockFreeMapExtractLast(token, extracted, value) != LOCK_FREE_MAP_CONFLICT || map.size() != 2)
        {
            return 12;
        }
        value.m_assign = nullptr;
        // 无条件提取遇到值被替换时重试并提取新值，不向调用方泄露值 CAS 冲突
        unsigned replacements = 0;
        /** 仅首次输出赋值时覆盖目标值，制造条目版本失效
        */
        std::function<void()> replace = [&map, &key, &replacements, &errors]()
        {
            if (replacements++ == 0)
            {
                errors += map.set(key, LockFreeMapStatusValue(15)) ? 0 : 1;
            }
        };
        value.m_assign = &replace;
        if (map.lockFreeMapExtract(key, value) != LOCK_FREE_MAP_COMMITTED || value.m_value != 15 ||
            replacements != 2 || map.contains(key))
        {
            return 13;
        }
        // 用户复制异常不应重试，也不能删除已经存在的条目
        unsigned exceptions = 0;
        /** 记录输出赋值次数后抛受控异常，验证未提交回滚
        */
        std::function<void()> fail = [&exceptions]()
        {
            ++exceptions;
            throw std::runtime_error("controlled output failure");
        };
        value.m_assign = &fail;
        if (map.lockFreeMapExtract(other, value) != LOCK_FREE_MAP_USER_EXCEPTION || exceptions != 1 || !map.contains(other))
        {
            return 14;
        }
        value.m_assign = nullptr;
        map.clearAndFreeMemory();
        LockFreeMapConfig config;
        config.nodes.initialCapacity = 1;
        config.nodes.maximumCapacity = 1;
        config.values.initialCapacity = 2;
        config.values.maximumCapacity = 2;
        config.operationRecordLimit = 1;
        if (map.lockFreeMapConfigure(config) != LOCK_FREE_MAP_OK || !map.set(key, LockFreeMapStatusValue(16)) ||
            map.lockFreeMapSet(other, LockFreeMapStatusValue(17)) != LOCK_FREE_MAP_CAPACITY)
        {
            return 15;
        }
        bool exhausted = false;
        // 活动借用确实占满保护记录，重入调用必须返回资源错误，不能等待自己归还
        /** 在唯一操作记录被外层借用占用时重入提取，记录资源拒绝
        @note 第1个未命名参数是同步借用输入，本回调不使用其内容
        */
        if (map.lockFreeMapVisit(key, [&map, &key, &exhausted](const LockFreeMapStatusValue&)
        {
            LockFreeMapStatusValue result;
            exhausted = map.lockFreeMapExtract(key, result) == LOCK_FREE_MAP_RECORD_LIMIT;
        }) != LOCK_FREE_MAP_OK || !exhausted || !map.contains(key))
        {
            return 16;
        }
        if (map.lockFreeMapExtract(key, value) != LOCK_FREE_MAP_COMMITTED || value.m_value != 16 ||
            map.lockFreeMapSet(other, LockFreeMapStatusValue(18)) != LOCK_FREE_MAP_COMMITTED)
        {
            return 17;
        }
        map.clearAndFreeMemory();
        if (errors != 0 || map.retainedNodeCount() != 0 || map.retainedValueCount() != 0)
        {
            return 18;
        }
        // 维护回调删除旧键后，原保护刚结束；容量一的池必须先回收再接纳新键
        LockFreeMap<int, int> bounded;
        LockFreeMapConfig boundedConfig;
        boundedConfig.nodes.initialCapacity = 1;
        boundedConfig.nodes.maximumCapacity = 1;
        boundedConfig.values.initialCapacity = 2;
        boundedConfig.values.maximumCapacity = 2;
        bool replaceDuringMaintenance = false;
        unsigned removedDuringMaintenance = 0;
        /** 在测试策略中一次性重入删除，制造维护与根变化交错
        @note 第1个未命名参数是同步借用输入，本回调不使用其内容
        @return 返回该受控场景的缩容决策；此重入仅用于白盒交错测试
        */
        boundedConfig.nodes.shrink = [&bounded, &replaceDuringMaintenance, &removedDuringMaintenance](const LockFreeMapShrinkContext&)
        {
            if (replaceDuringMaintenance)
            {
                replaceDuringMaintenance = false;
                if (bounded.erase(0))
                {
                    ++removedDuringMaintenance;
                }
            }
            return LockFreeMapShrinkDecision();
        };
        if (bounded.lockFreeMapConfigure(boundedConfig) != LOCK_FREE_MAP_OK || !bounded.set(0, 10))
        {
            return 20;
        }
        replaceDuringMaintenance = true;
        if (bounded.lockFreeMapSet(1, 20) != LOCK_FREE_MAP_COMMITTED || removedDuringMaintenance != 1 ||
            bounded.contains(0) || bounded.size() != 1)
        {
            return 21;
        }
        int boundedValue = 0;
        if (!bounded.get(1, boundedValue) || boundedValue != 20)
        {
            return 22;
        }
        bounded.clearAndFreeMemory();
        return bounded.retainedNodeCount() == 0 && bounded.retainedValueCount() == 0 ? 0 : 23;
    }
    catch (...)
    {
        return 19;
    }
}