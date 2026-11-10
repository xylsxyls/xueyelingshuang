#pragma once

#include <list>
#include <mutex>
#include <stdint.h>
#include <type_traits>
#include <utility>

// Isolated benchmark substitute, never included with the production Queue.
// The frozen benchmark names this header LockFreeQueue.h so every version uses
// exactly the same workload. One ordinary mutex protects all list operations.
template<class T>
class LockFreeQueue
{
public:
    /** 创建空列表和互斥量；无独立内存池或生产者登记
    */
    LockFreeQueue();

    /** 外部静止后析构列表及其中载荷
    */
    ~LockFreeQueue();

    /** 清空已有元素；列表和互斥量对象保留，基线没有池生命周期接口
    */
    void init();

    /** 清空已有元素；物理列表对象在析构时结束
    */
    void destroy();

    /** 持有同一个互斥量，清空列表
    */
    void clear();

    /** 持锁复制到列表末尾；构造失败传播异常且不插入元素
    @param [in] value 调用者持有的原值，借用至返回
    */
    void push(const T& value);

    /** 持锁取走首项；赋值成功后才移除，nullptr只丢弃一项
    @param [out] output 接收载荷，可为nullptr；赋值异常可能部分改变输出
    @return 是否成功移除一个元素
    */
    bool pop(T* output);

    /** 持锁观察列表是否为空；测试用非const入口避免mutable成员
    @return 当前列表是否为空
    */
    bool empty();

    /** 持锁读取列表元素数
    @return 当前列表元素数
    */
    uint64_t size();

private:
    // 实际被比较的标准库容器，不引入自定义分配器或缓存
    std::list<T> m_items;
    // 全部操作共用的标准互斥量
    std::mutex m_mutex;

private:
    /** 与当前Queue一致：保证不抛异常的移动赋值优先
    @param [out] output 接收载荷
    @param [in,out] source 仍由列表持有的首项
    */
    static void assign(T& output, T& source, std::true_type);

    /** 移动可能抛异常时采用const复制，异常保留列表项
    @param [out] output 接收载荷
    @param [in,out] source 仍由列表持有的首项
    */
    static void assign(T& output, T& source, std::false_type);

    /** 禁止复制包含互斥量的队列
    @param [in] other 禁止复制的源对象
    */
    LockFreeQueue(const LockFreeQueue& other) = delete;

    /** 禁止复制包含互斥量的队列
    @param [in] other 禁止复制的源对象
    */
    LockFreeQueue& operator=(const LockFreeQueue& other) = delete;
};

#include "LockFreeQueueListMutex.inl"