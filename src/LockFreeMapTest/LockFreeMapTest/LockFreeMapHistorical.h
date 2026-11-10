#pragma once
#include <stdint.h>

/** 历史兼容性用例专用故障开关，仅由单线程故障阶段设置
*/
struct LockFreeMapHistoricalFaults
{
public:
    // 键复制和赋值故障
    static bool m_keyCopy;
    // 值复制故障
    static bool m_valueCopy;
    // 值赋值故障
    static bool m_valueAssign;
    // 比较器故障
    static bool m_less;
};

/** 历史用例中的可注入复制故障的键
*/
struct LockFreeMapHistoricalKey
{
public:
    // 键的独立比较值
    int32_t m_value;

public:
    /** 构造键
    @param [in] value 键值
    */
    explicit LockFreeMapHistoricalKey(int32_t value = 0);

    /** 复制键并检查故障开关
    @param [in] other 原键
    */
    LockFreeMapHistoricalKey(const LockFreeMapHistoricalKey& other);

    /** 赋值并检查故障开关
    @param [in] other 原键
    @return 当前键
    */
    LockFreeMapHistoricalKey& operator=(const LockFreeMapHistoricalKey& other);
};

/** 历史用例中的可注入复制和赋值故障的值
*/
struct LockFreeMapHistoricalValue
{
public:
    // 独立有效载荷
    int32_t m_value;

public:
    /** 构造有效载荷
    @param [in] value 载荷内容
    */
    explicit LockFreeMapHistoricalValue(int32_t value = 0);

    /** 复制并检查故障开关
    @param [in] other 原载荷
    */
    LockFreeMapHistoricalValue(const LockFreeMapHistoricalValue& other);

    /** 赋值并检查故障开关
    @param [in] other 原载荷
    @return 当前载荷
    */
    LockFreeMapHistoricalValue& operator=(const LockFreeMapHistoricalValue& other);
};

/** 历史用例的可抛异常比较器
*/
struct LockFreeMapHistoricalLess
{
public:
    /** 根据数值比较，同时支持比较故障注入
    @param [in] left 左键
    @param [in] right 右键
    @return 左键严格小于右键时返回 true
    */
    bool operator()(const LockFreeMapHistoricalKey& left, const LockFreeMapHistoricalKey& right) const;
};