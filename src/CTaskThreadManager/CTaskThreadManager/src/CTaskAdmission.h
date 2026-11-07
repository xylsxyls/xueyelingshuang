#pragma once
#include <atomic>
#include <stdint.h>

/** 可关闭的并发接纳计数；不互斥调用者，不等待某个独占者释放
*/
class CTaskAdmission
{
public:
    /** 创建开放且无人使用的接纳门
    */
    CTaskAdmission();

    /** 尝试登记并发调用
    @return 已关闭或计数耗尽时返回 false
    */
    bool enter();

    /** 归还一次成功 enter 的登记
    */
    void leave();

    /** 禁止后来的 enter；已有调用仍需由生命周期 owner 等待
    */
    void close();

    /** 仅在静止状态重开接纳
    */
    void reopen();

    /** 仅在无人使用时关闭，用于退役空桶
    @return 成功关闭时为 true
    */
    bool closeIfEmpty();

    /** 观察已登记调用数
    @return 未归还登记数量
    */
    uint64_t active() const;

    /** 查询是否关闭
    @return 不再接纳新登记时为 true
    */
    bool closed() const;

    /** 检查计数原子是否无锁
    @return 后端是否满足无锁要求
    */
    bool supported() const;

private:
    // 最高位为关闭状态，其余位是并发登记数
    std::atomic<uint64_t> m_state;
};

#include "CTaskAdmission.inl"