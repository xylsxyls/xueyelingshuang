#pragma once
#include "SemaphoreEventPool.h"

class SemaphoreWake;

/** 本地计数的慢路径：同一原子字表示积存数或等待链，避免登记与直接交付之间的空隙
    低于2^63的积存计数直接存储；最高位为1时保存受读者保护的记录地址。
    地址至少4字节对齐且非空，最低位区分等待链和不可变大计数记录；大计数仍支持完整UINT64_MAX范围。
    私有继承仅复用记录分配、读者保护和回收，不使用事件登记链。
    本类只管理内部记录，不是业务接入接口；析构和clear要求外部静止。
*/
class SemaphoreCount : private SemaphoreEventPool
{
public:
    /** 准备计数等待专用记录池，与公开事件池的配置和预算相互独立
    */
    explicit SemaphoreCount(const SemaphorePoolConfig& config = SemaphorePoolConfig());

    /** 取得许可或登记等待；probe不登记，已有等待者时不能插队
    @param [in,out] state Semaphore持有的原子状态
    @param [out] record 非空表示已登记，调用者必须finish
    @param [in] probe 是否仅立即探测
    @return OK且committed表示立即消费，否则OK表示已登记；失败不消费
    */
    SemaphoreResult begin(std::atomic<uint64_t>& state, SemaphoreEventRecord*& record, bool probe);

    /** 优先直接交付最早等待者；没有合格等待者时才积存
    @param [in,out] state Semaphore持有的原子状态
    @param [in,out] wake 本地系统等待后端
    @param [in] notify 是否立即提示，false仅供组合系统调用随后统一提示
    @return committed表示许可已交付或积存，提示失败不能重复发布
    */
    SemaphoreResult signal(std::atomic<uint64_t>& state, SemaphoreWake& wake, bool notify = true);

    /** 与直接交付竞争取消，已交付许可不被后来线程夺走
    @param [in,out] state Semaphore持有的原子状态
    @param [in] record 本次已登记的记录
    @return true表示发布者已交付许可，否则取消成功
    */
    bool finish(std::atomic<uint64_t>& state, SemaphoreEventRecord* record);

    /** 静止期清除积存数及内部记录，不与等待、通知并发
    @param [in,out] state Semaphore持有的原子状态
    */
    void clear(std::atomic<uint64_t>& state);

    /** 计数等待专用池的观测值，不包含事件池
    @return 已保留、使用中、退役及预留容量
    */
    SemaphorePoolStats statistics() const;

    /** 静止空闲期按配置维护计数记录池
    @param [out] released 物理释放记录数
    @return 维护结果
    */
    SemaphoreResult maintain(size_t& released);

    /** 可直接存储的最大积存数，大于此值仍通过不可变记录表示
    @return 2^63-1
    */
    static uint64_t directLimit();

private:
    /** 在读者保护内取得未发布记录，失败不改变计数
    @param [out] record 取得后须发布或discard
    @return OK或分配/地址表示错误
    */
    SemaphoreResult acquire(SemaphoreEventRecord*& record);

    /** 把独占记录交给延迟回收，当前调用仍须持有读者保护或处于静止期
    @param [in] record 不再由等待者访问的记录
    @param [in] large 是否保留大计数正文至全部读者退出后才改作空闲时间
    */
    void discard(SemaphoreEventRecord* record, bool large = false);

    /** 读者保护内的等待入口，退出由外层统一释放保护
    @param [in,out] state 计数或等待链
    @param [out] record 已登记记录，立即消费时为空
    @param [in] probe 是否仅探测
    @return 本次登记或消费结果
    */
    SemaphoreResult beginProtected(std::atomic<uint64_t>& state, SemaphoreEventRecord*& record, bool probe);

    /** 读者保护内的发布入口
    @param [in,out] state 计数或等待链
    @param [in,out] wake 提示后端
    @param [in] notify 是否立即系统提示
    @return 本次提交及提示结果
    */
    SemaphoreResult signalProtected(std::atomic<uint64_t>& state, SemaphoreWake& wake, bool notify);

    /** 替换不可变大计数记录，CAS失败时回收候选并返回RETRY
    @param [in,out] state 共享状态
    @param [in] expected 受保护的旧状态
    @param [in] value 期望积存数
    @return OK且committed表示替换成功，其余不改变共享状态
    */
    SemaphoreResult replaceLarge(std::atomic<uint64_t>& state, uint64_t expected, uint64_t value);

    /** 帮助摘除已交付或取消记录，暂停等待者不会阻止后续许可发布
    @param [in,out] state 计数或等待链
    */
    void sweep(std::atomic<uint64_t>& state);

    /** 解码已验证的记录地址；调用者须有读者保护
    @param [in] state 带最高位标记的状态
    @return 记录地址
    */
    static SemaphoreEventRecord* node(uint64_t state);

    /** 编码记录地址；acquire已核实可表示性
    @param [in] record 记录地址
    @param [in] large 是否大计数记录
    @return 带类型标记的原子状态
    */
    static uint64_t encode(SemaphoreEventRecord* record, bool large = false);

};