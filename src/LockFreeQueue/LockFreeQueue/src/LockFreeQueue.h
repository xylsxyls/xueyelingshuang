#pragma once

#include <atomic>
#include <cstdint>
#include <cassert>
#include <thread>
#include <type_traits>
#include <new>
#include <utility>

template<class QueueElementType>
struct QueueNode
{
	std::atomic<QueueNode*> m_next;
	/*
	C++11 手写 optional 存储。
	不直接使用 QueueElementType m_data，原因：
	1. 哨兵节点不构造 QueueElementType；
	2. 不要求 QueueElementType 必须有默认构造函数；
	3. pop 后，数据节点会变成新的哨兵节点，数据需要立刻析构；
	4. 不额外 new QueueElementType，数据直接放在 QueueNode 内部。
	*/
	typename std::aligned_storage<sizeof(QueueElementType), std::alignment_of<QueueElementType>::value>::type m_storage;

	bool m_hasData;

	// 构造哨兵节点，不构造 QueueElementType。
	QueueNode();

	// 构造数据节点，在节点内部 placement new QueueElementType。
	explicit QueueNode(const QueueElementType& data);

	~QueueNode();

	QueueNode(const QueueNode&) = delete;
	QueueNode& operator=(const QueueNode&) = delete;

	QueueElementType* data_ptr();
	const QueueElementType* data_ptr() const;

	void construct_data(const QueueElementType& data);
	void destroy_data();
};

template<class QueueElementType>
class LockFreeQueue
{
public:
	LockFreeQueue();
	~LockFreeQueue();

	LockFreeQueue(const LockFreeQueue&) = delete;
	LockFreeQueue& operator=(const LockFreeQueue&) = delete;

public:
	// 重置队列：必须在无并发时调用。
	// 如果队列里原来有内容，会先销毁旧队列再重新初始化。
	void init();

	// 清空队列：
	// 可以与 push 并发；
	// 不能和 pop 并发；
	// 不能多个线程同时 clear。
	void clear();

	// 销毁队列：
	// 必须在无并发时调用。
	// destroy 后可以再次 init 继续使用。
	void destroy();

	// 多生产者线程安全。
	// 注意：接口是 const T&，所以保存元素时必然会发生一次 T 的拷贝构造。
	void push(const QueueElementType& e);

	// 单消费者调用。
	// e != nullptr：弹出元素并写入 *e。
	// e == nullptr：弹出元素但不返回数据，直接丢弃。
	//
	// 不能多个线程同时 pop；
	// 不能和 clear 并发。
	bool pop(QueueElementType* e);

	// 并发下是近似判断；
	// 无并发或者所有 push/pop/clear 结束后准确。
	bool empty() const;

	// 并发下是近似值；
	// 无并发或者所有 push/pop/clear 结束后准确。
	int32_t size() const;

private:
	// 当前消费者侧 head，指向哨兵节点。
	// 只有单消费者执行流会访问和推进它，所以不需要 atomic。
	QueueNode<QueueElementType>* m_head;

	// 当前生产者侧 tail。
	// 多生产者通过 exchange 原子推进它。
	std::atomic<QueueNode<QueueElementType>*> m_tail;

	// 近似元素个数。
	std::atomic<int32_t> m_count;
};

#include "LockFreeQueue.inl"