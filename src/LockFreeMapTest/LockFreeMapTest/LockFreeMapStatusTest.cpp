#include "LockFreeMapStatusTest.h"
#include "LockFreeMapTest.h"
#include "LockFreeMap/LockFreeMap.h"
#include <stdexcept>

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

int LockFreeMapStatusContract()
{
    try
    {
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
                settings.nodes.shrink = [](const LockFreeMapShrinkContext&)
                {
                    return LockFreeMapShrinkDecision(4, 1);
                };
            }
            if ((flags & 2) != 0)
            {
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