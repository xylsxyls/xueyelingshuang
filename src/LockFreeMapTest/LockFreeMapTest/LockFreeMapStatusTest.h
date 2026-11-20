#pragma once
#include <functional>
#include <cstddef>
#include <stdint.h>

struct LockFreeMapGrowContext;
struct LockFreeMapShrinkContext;
struct LockFreeMapShrinkDecision;

/** 在配置复制策略时触发指定异常，验证替换失败后的原策略
*/
struct LockFreeMapPolicyCopyFault
{
public:
    /** 选择复制时使用的异常
    @param [in] allocation true 表示内存不足，false 表示用户异常
    */
    explicit LockFreeMapPolicyCopyFault(bool allocation);

    /** 复制时抛出源对象指定的异常
    @param [in] other 待复制的策略
    */
    LockFreeMapPolicyCopyFault(const LockFreeMapPolicyCopyFault& other);

    /** 拒绝增长，便于识别错误替换
    @param [in] context 增长上下文
    @return 零
    */
    std::size_t operator()(const LockFreeMapGrowContext& context) const;

    /** 返回关闭缩容的决策，供缩容策略复制失败用例使用
    @param [in] context 本次缩容观察
    @return 零释放量和零空闲时间
    */
    LockFreeMapShrinkDecision operator()(const LockFreeMapShrinkContext& context) const;

    /** 空通知，错误替换原通知时不会累计增长数
    @param [in] added 本次新增槽数
    @param [in] count 累计增长次数
    */
    void operator()(std::size_t added, uint64_t count) const;

private:
    // 复制时是否模拟内存不足
    bool m_allocation;
};

/** 保持整数严格排序，在查询比较时触发独立操作；存入容器的副本不携带回调
*/
struct LockFreeMapStatusKey
{
public:
    // 排序值
    int m_value;
    // 仅借用调用栈上的测试回调
    std::function<void()>* m_compare;

public:
    /** 创建查询键或普通键
    @param [in] value 排序值
    @param [in] compare 可选比较回调，使用期间必须存活
    */
    explicit LockFreeMapStatusKey(int value = 0, std::function<void()>* compare = nullptr);

    /** 复制纯键值，不把交错回调写入容器
    @param [in] other 源键
    */
    LockFreeMapStatusKey(const LockFreeMapStatusKey& other);

    /** 复制输出键值，不传播交错回调
    @param [in] other 源键
    @return 当前键引用
    */
    LockFreeMapStatusKey& operator=(const LockFreeMapStatusKey& other);

    /** 执行受控交错，排序关系始终只取决于整数
    @param [in] other 比较键
    @return 当前整数小于另一个整数时返回 true
    */
    bool operator<(const LockFreeMapStatusKey& other) const;
};

/** 输出对象通过赋值回调控制复制到提交之间的交错，不在库中嵌入测试分支
*/
struct LockFreeMapStatusValue
{
public:
    // 值内容
    int m_value;
    // 仅输出对象借用的回调，容器副本不携带回调
    std::function<void()>* m_assign;

public:
    /** 创建普通值或带赋值回调的输出对象
    @param [in] value 值内容
    @param [in] assign 可选赋值回调，使用期间必须存活
    */
    explicit LockFreeMapStatusValue(int value = 0, std::function<void()>* assign = nullptr);

    /** 复制纯值内容
    @param [in] other 源值
    */
    LockFreeMapStatusValue(const LockFreeMapStatusValue& other);

    /** 执行目标对象的赋值回调并复制内容
    @param [in] other 源值
    @return 当前值引用
    */
    LockFreeMapStatusValue& operator=(const LockFreeMapStatusValue& other);

    /** 作为有序键时只比较整数内容
    @param [in] other 参与比较的另一项
    @return 当前整数小于另一项时true
    */
    bool operator<(const LockFreeMapStatusValue& other) const;
};

/** 检查普通写入的内部重试、重叠参数、条件失效和资源错误
@return 独立预期全部满足时返回零
*/
int LockFreeMapStatusContract();