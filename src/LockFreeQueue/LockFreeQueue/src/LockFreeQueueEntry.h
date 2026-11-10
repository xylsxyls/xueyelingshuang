#pragma once

#include <stdint.h>

/** 调用方拥有的外部载荷；成功入队后保持地址和字段不变，直到消费返回或静止 destroy
*/
template <class T>
#if defined(_MSC_VER)
struct __declspec(align(8)) LockFreeQueueEntry
#else
struct alignas(alignof(T) > 8 ? alignof(T) : 8) LockFreeQueueEntry
#endif
{
public:
    // 调用方构造和析构；值输出接口复制临时对象，地址输出接口不访问 T；队列不移动或析构本值
    T m_value;
    // 在队列内由队列独占，用于归还节点池额度；未入队或消费返回后可由 owner 复用
    uint64_t m_cookie;
    // 在队列内由队列独占的额度所属块；与 cookie 一起避免消费时读取额度节点
    void* m_owner;

    /** 创建未入队的外部载荷；复制构造异常由创建者处理
    @param [in] value 初始载荷
    */
    explicit LockFreeQueueEntry(const T& value = T());

    /** 禁止复制，避免误复制处于队列内的额度身份
    @param [in] other 不允许复制的源对象
    */
    LockFreeQueueEntry(const LockFreeQueueEntry& other) = delete;

    /** 禁止赋值，载荷修改只允许在未入队或消费已经返回时进行
    @param [in] other 不允许复制的源对象
    */
    LockFreeQueueEntry& operator=(const LockFreeQueueEntry& other) = delete;

};

#include "LockFreeQueueEntry.inl"