#pragma once
#include <memory>
#include <functional>
#include <type_traits>

/** 策略调用接口，由共享句柄拥有，避免 v120 的 std::function::_Copy 异常泄漏
*/
template <typename ResultType, typename ContextType>
class LockFreeHashMapPolicyFunction
{
public:
    /** 释放用户回调，要求其析构不抛异常
    */
    virtual ~LockFreeHashMapPolicyFunction();

    /** 调用用户提供的线程安全策略，异常交由池评估边界处理
    @param [in] context 调用期间借用的弱一致容量观测
    @return 用户策略请求，由池进一步检查容量和安全性
    */
    virtual ResultType invoke(const ContextType& context) = 0;
};

/** 一次异常安全的申请拥有完整回调，构造失败自动释放申请
*/
template <typename ResultType, typename ContextType, typename FunctionType>
class LockFreeHashMapPolicyHolder : public LockFreeHashMapPolicyFunction<ResultType, ContextType>
{
public:
    /** 在 make_shared 构造中复制回调，异常时释放整个申请
    @param [in] function 待复制的回调，目标析构不得抛异常
    */
    explicit LockFreeHashMapPolicyHolder(const FunctionType& function);

    /** 调用持有的回调，可变状态必须自行保证线程安全
    @param [in] context 调用期间借用的容量观测
    @return 用户策略请求，异常向池评估边界传播
    */
    virtual ResultType invoke(const ContextType& context);
private:
    // 独占回调对象，配置池时只共享外层句柄，不复制此对象
    FunctionType m_function;
};

/** 可复制的策略所有权句柄，回调自身可能被并发调用
准备回调的分配和复制异常由调用方处理，池评估捕获回调执行异常
*/
template <typename ResultType, typename ContextType>
class LockFreeHashMapPolicy
{
public:
    /** 构造空句柄，表示使用库的默认策略
    */
    LockFreeHashMapPolicy();

    /** 共享所有权，不申请内存或复制用户回调
    @param [in] other 同步借用的源句柄，不得同时修改它
    */
    LockFreeHashMapPolicy(const LockFreeHashMapPolicy& other);

    /** 替换所有权，不申请内存或复制用户回调
    @param [in] other 同步借用的源句柄，不得同时修改它
    @return 当前句柄，仅外部静止时修改已安装的策略
    */
    LockFreeHashMapPolicy& operator=(const LockFreeHashMapPolicy& other);

    /** 复制并持有 lambda 或函数对象
    @param [in] function 待复制回调，分配和复制异常由调用方捕获
    */
    template <typename FunctionType>
    LockFreeHashMapPolicy(const FunctionType& function);

    /** 持有普通函数指针，分配异常由调用方处理
    @param [in] function 策略函数，接收调用期观测并返回请求；空指针构造空策略
    */
    LockFreeHashMapPolicy(ResultType (*function)(const ContextType& context));

    /** 禁止调用 v120 不安全的类型擦除回调复制，改用 fromShared
    @param [in] function 不可直接复制的 std::function 对象
    */
    template <typename SignatureType>
    LockFreeHashMapPolicy(const std::function<SignatureType>& function) = delete;

    /** 共享已有回调或 std::function，不复制目标回调
    @param [in] function 共享所有权句柄，空值构造空策略
    @return 持有源回调的策略，准备包装时的分配异常由调用方处理
    */
    template <typename FunctionType>
    static LockFreeHashMapPolicy fromShared(const std::shared_ptr<FunctionType>& function);

    /** 查询是否安装了用户策略
    @return 非空为 true，不执行回调
    */
    explicit operator bool() const;

    /** 执行策略，容器在评估边界捕获异常
    @param [in] context 调用期间借用的弱一致观测
    @return 策略请求，空句柄抛 bad_function_call，直接调用者负责捕获
    */
    ResultType operator()(const ContextType& context) const;

    /** 外部静止时交换策略所有权句柄
    @param [in,out] other 参与交换的句柄，不执行或复制回调
    */
    void swap(LockFreeHashMapPolicy& other);
private:
    // 固定所有权，复制配置时共享，不在运行中更换
    std::shared_ptr<LockFreeHashMapPolicyFunction<ResultType, ContextType>> m_function;
};

#include "LockFreeHashMapPolicy.inl"