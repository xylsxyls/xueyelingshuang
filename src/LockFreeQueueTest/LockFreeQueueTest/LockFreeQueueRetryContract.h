#pragma once
#include "LockFreeQueueAPI.h"

/** 复制异常载荷；开关仅由静止测试线程使用 */
class LockFreeQueueRetryValue
{
public:
    // 是否在复制构造中抛出
    static bool m_failCopy;
    // 独立校验值
    int32_t m_value;

    /** 创建载荷
    @param [in] value 校验值
    */
    explicit LockFreeQueueRetryValue(int32_t value);

    /** 复制载荷或注入异常
    @param [in] other 原值
    */
    LockFreeQueueRetryValue(const LockFreeQueueRetryValue& other);
};

/** 测试std::function复制失败时配置回滚 */
class LockFreeQueueRetryObserver
{
public:
    /** 借用静止测试的失败开关
    @param [in] failCopy 复制时检查，寿命覆盖配置调用
    */
    explicit LockFreeQueueRetryObserver(const bool& failCopy);

    /** 复制回调或抛出测试异常
    @param [in] other 原回调
    */
    LockFreeQueueRetryObserver(const LockFreeQueueRetryObserver& other);

    /** 空观察者
    @param [in] amount 新增槽数
    @param [in] count 累计增长快照
    */
    void operator()(size_t amount, uint64_t count) const;
private:
    // 借用期由测试持有者保证
    const bool* m_failCopy;
};

/** 重试及新增公开接口回归；等待观察钩子只注入隔离白盒副本
1. 四种开关组合：未开启容量等待时满池返回CapacityLimit，开启后消费释放即可成功
2. 普通push及保留生产者、Fence和Entry入口：等待过程中不丢失或重复提交
3. 容量退避：前4096次不休眠，之后每次等待1毫秒；计数饱和不回绕
4. 四生产者固定容量压力：每线程FIFO，8000个元素精确一次
5. 未初始化及载荷复制异常不因开启两项重试而循环
6. 扩容通知的发布、复用、拒绝、回调异常和配置复制失败回滚
7. Entry前缀的固定/动态池、预约开关、边界预算、跨段复用及丢弃
8. 单线程Wasm明确返回容量等待不支持，不创建假线程
独立程序超时由测试运行器终止并判失败，不能把超时视为通过
*/
class LockFreeQueueRetryContract
{
public:
    /** 执行全部重试契约用例
    @return true全部通过
    */
    bool run();

    /** 验证消费者协作阈值的合法性、失败不改值、重初始化保留及整批结算
    @return true表示实际yield调用数及队列状态符合公开契约
    */
    bool consumerYield();

    /** 验证长Entry前缀的固定及动态池、预约模式、所有段内起点和输出边界
    @return true表示每个地址、数量、FIFO及额度复用均正确
    */
    bool entrySpan();

private:
    /** 验证单项阈值下256项整批消费只在两个记账点让步
    @return true表示消费256项且实际执行两次消费者yield
    */
    bool consumerBatchBudget();

    /** 验证不同提交入口与两项开关的满载行为
    @param [in] mode 0..7为普通/上下文的值、Fence、Entry、EntryFence；8/9为普通push
    @param [in] retryContention 是否持续重试竞争
    @param [in] retryCapacity 是否持续等待容量
    @return true结果及实际消息完整
    */
    bool capacity(size_t mode, bool retryContention, bool retryCapacity);

    /** 验证容量等待计数及每次延迟
    @return true符合4096次切换和计数饱和规则
    */
    bool delay();

    /** 验证有界池多生产者并发等待后完整消费
    @return true没有丢失、重复或单线程乱序
    */
    bool concurrent();

    /** 验证消费者提前失败时仍排空满池，使正在等待容量的原生产线程自行返回
    @return true停止标志、三条生产线程完成数及最终空状态均正确
    */
    bool cancelledConsumer();

    /** 验证四种重试组合的复制异常及失败后恢复
    @return true错误码和队列状态正确
    */
    bool exceptions();

    /** 验证扩容通知及配置复制失败回滚
    @return true通知和容量一致
    */
    bool growth();

    /** 验证并发扩容回调逐次增量和增长统计一致
    @return true四生产者载荷及所有通知均完整
    */
    bool growthConcurrent();

    /** 验证登记槽满载时两项重试开关独立生效
    @return true释放原登记后新登记成功且无泄漏
    */
    bool registration();

    /** Verify the parking decision against real spare capacity and registration history
    @return True for finite and unlimited pools at the 1048576-node per-participant boundary
    */
    bool publisherCapacity();

    /** Verify reservation selection after lease reuse and quiescent initialization
    @return FIFO, record stride, release and reset retain their existing contracts
    */
    bool reservationReuse();

    /** 验证前缀地址、FIFO、预算及额度复用
    @return true四种配置及非Entry配置均正确
    */
    bool prefix();

    /** Check empty reserved observations, new publisher registration and consumer seals
    @return True when unpublished work is not consumed and later published work remains reachable
    */
    bool reservationProgress();

    /** Exercise sole-publisher handoff before and after publication at real boundaries
    @return True when the owner completes its own payload and the consumer never awaits it
    */
    bool solePublisherTransition();

    /** The joining publisher remains paused while the original publishes beyond its hole
    @return True when both owners complete and the consumer crosses the hole without waiting
    */
    bool solePublisherHole();

    /** Verify value-prefix FIFO, output bounds, zero budgets and assignment recovery
    @return True for fixed/dynamic pools, both reservation modes and shallow/batched inputs
    */
    bool valuePrefix();
};