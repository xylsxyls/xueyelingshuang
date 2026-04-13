#pragma once
#include <atomic>
#include <cstddef>
#include <stdint.h>
#include <vector>

template<class QueueElmentType>
struct QueueNode;

template<class QueueElmentType>
struct VersionPtr
{
	QueueNode<QueueElmentType>* m_ptr;
	size_t m_version;

	VersionPtr();
	bool operator==(const VersionPtr& o) const;
	bool operator!=(const VersionPtr& o) const;
};

template<class QueueElmentType>
struct QueueNode
{
	std::atomic<QueueNode*> m_next;
	QueueElmentType m_data;

	QueueNode();
};

template<class QueueElmentType>
class LockFreeQueue
{
public:
	LockFreeQueue();
	~LockFreeQueue();

public:
	// 重置队列（必须在无并发时调用，或作为内部使用）
	void init();
	// 线程安全清空（可与 push 并发，但必须在单消费者线程中调用）
	void clear();
	// 销毁所有节点（必须在无并发时调用）
	void destroy();
	void push(const QueueElmentType& e);
	bool pop(QueueElmentType* e);
	bool empty() const;
	int32_t size() const;

private:
	// 垃圾回收：由 pop 线程定期调用以释放已出队节点（也可不调，析构时会清理）
	void gc();
	// 非线程安全清空（内部使用）
	void clear_unsafe();
	// 将节点加入待删除列表（仅在 pop 线程调用）
	void retire_node(QueueNode<QueueElmentType>* node);

private:
	std::atomic<VersionPtr<QueueElmentType>> m_front;
	std::atomic<VersionPtr<QueueElmentType>> m_rear;
	// 版本号生成器
	std::atomic<size_t> m_tag;
	// 近似元素个数
	std::atomic<int32_t> m_count;

	// 仅由单消费者线程使用，无需原子操作
	std::vector<QueueNode<QueueElmentType>*> m_retired;
};

#include "LockFreeQueue.inl"