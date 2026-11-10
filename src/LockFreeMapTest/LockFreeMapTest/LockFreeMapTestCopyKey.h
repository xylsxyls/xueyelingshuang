#pragma once
#include <atomic>

/** 直接构造优化的独立复制次数观察器
*/
struct LockFreeMapTestCopyKey
{
public:
    // 键复制构造的实际次数
    static std::atomic<int> m_copies;
    // 是否在复制构造中抛出受控异常
    static bool m_throw;
    // 独立键值
    int m_value;

public:
    /** 构造源键，不计入复制次数
    @param [in] value 键值
    */
    explicit LockFreeMapTestCopyKey(int value);

    /** 计数并执行可能失败的复制
    @param [in] other 原键
    */
    LockFreeMapTestCopyKey(const LockFreeMapTestCopyKey& other);

    /** 为边界查询提供普通赋值，不计入构造次数
    @param [in] other 原键
    @return 当前对象
    */
    LockFreeMapTestCopyKey& operator=(const LockFreeMapTestCopyKey& other);

    /** 以独立键值建立严格弱序
    @param [in] other 右操作数
    @return 当前键严格小于右键时返回 true
    */
    bool operator<(const LockFreeMapTestCopyKey& other) const;
};