#pragma once
#include <functional>

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
};

/** 检查普通接口内部重试、条件失效和真实容量错误的区分
@return 独立预期全部满足时返回零
*/
int LockFreeMapStatusContract();