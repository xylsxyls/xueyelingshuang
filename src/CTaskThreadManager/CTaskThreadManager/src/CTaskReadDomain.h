#pragma once
#include <atomic>
#include <stddef.h>
#include <stdint.h>

/** 一个同时借用者独占的公布位置；零表示没有访问受保护对象
*/
struct CTaskReadPosition
{
public:
    // CAS 同时取得借用位置并公布 epoch；不等待某个占有者
    std::atomic<uint64_t> m_epoch;
    // 避免不同生产者在同一缓存行反复写入
    unsigned char m_padding[56];

public:
    /** 建立未借用状态
    */
    CTaskReadPosition();

};

/** 稳定地址记录的读侧保护；借用期间允许回调，暂停借用者只延迟回收
*/
class CTaskReadDomain
{
public:
    /** 构造空域，实际存储由 initialize 分配
    */
    CTaskReadDomain();

    /** 外部保证所有借用结束后释放记录表
    */
    ~CTaskReadDomain();

    /** 静止初始化固定数量借用位置；失败不公布半成品
    @param [in] count 并发借用及重入的总预算，至少为 1
    @return 是否分配完成且所需原子类型实际无锁
    */
    bool initialize(size_t count);

    /** 检查读侧登记及 epoch 原子宽度，供初始化区分平台不支持和分配失败
    @return 所需原子操作是否均为无锁
    */
    bool atomicsLockFree() const;

    /** 取得一个借用位置并验证 epoch；拒绝关闭域或已用尽的保护预算
    @return 有效位置或 nullptr；返回前不得解引用受保护对象
    */
    CTaskReadPosition* enter();

    /** 尝试借用指定位置，供唯一维护者清理空闲位置的私有缓存
    @param [in] index 小于 capacity 的位置索引
    @return 成功取得的位置或 nullptr，不等待已有借用者
    */
    CTaskReadPosition* tryEnter(size_t index);

    /** 将本域返回的位置映射到并行缓存下标
    @param [in] position 本域 enter 或 tryEnter 返回的位置
    @return 固定位置下标
    */
    size_t indexOf(const CTaskReadPosition& position) const;

    /** 归还本调用独占的借用位置
    @param [in,out] position enter 返回的非空位置
    */
    void leave(CTaskReadPosition& position);

    /** 关闭后拒绝新借用；记录表仍保留到本对象析构
    */
    void close();

    /** 查询是否已经关闭
    @return 是否关闭
    */
    bool closed() const;

    /** 查询在途借用数量，关闭后用于收敛
    @return 弱一致数量
    */
    size_t active() const;

    /** 唯一回收者推进 epoch，不能回绕
    @return 推进前的 epoch；零表示代次已耗尽
    */
    uint64_t advance();

    /** 唯一回收者取得尚被读者保护的最小 epoch
    @return 所有非零公布值的最小值，没有读者时为 UINT64_MAX
    */
    uint64_t oldest() const;

    /** 查询当前 epoch，退休记录时保存此值
    @return 非零 epoch
    */
    uint64_t current() const;

    /** 查询固定位置数量
    @return initialize 成功时的 count
    */
    size_t capacity() const;

private:
    /** 禁止复制仍在使用的保护域
    @param [in] other 源对象
    */
    CTaskReadDomain(const CTaskReadDomain& other) = delete;

    /** 禁止复制仍在使用的保护域
    @param [in] other 源对象
    @return 本对象
    */
    CTaskReadDomain& operator=(const CTaskReadDomain& other) = delete;

private:
    // 64 字节对齐位置表拥有的原始存储
    unsigned char* m_storage;
    // 从原始存储得到的对齐位置表
    CTaskReadPosition* m_positions;
    // 固定预算，初始化成功后不变
    size_t m_count;
    // 只有唯一回收者写入，读侧验证保护开始与回收的顺序
    std::atomic<uint64_t> m_epoch;
    // 只从开放变为关闭；关闭不释放借用位置表
    std::atomic<bool> m_closed;
};

#include "CTaskReadDomain.inl"