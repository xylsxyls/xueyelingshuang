#ifndef _LOCK_FREE_QUEUE_H__
#define _LOCK_FREE_QUEUE_H__
#include "LockFreeQueue.h"

// ==================== QueueNode 实现 ====================
template<class QueueElmentType>
QueueNode<QueueElmentType>::QueueNode() :
m_next(nullptr)
{

}

// ==================== VersionPtr 实现 ====================
template<class QueueElmentType>
VersionPtr<QueueElmentType>::VersionPtr() :
m_ptr(nullptr),
m_version(0)
{

}

template<class QueueElmentType>
bool VersionPtr<QueueElmentType>::operator==(const VersionPtr& o) const
{
	return m_ptr == o.m_ptr && m_version == o.m_version;
}

template<class QueueElmentType>
bool VersionPtr<QueueElmentType>::operator!=(const VersionPtr& o) const
{
	return !(*this == o);
}

// ==================== LockFreeQueue 实现 ====================
template<class QueueElmentType>
LockFreeQueue<QueueElmentType>::LockFreeQueue() :
m_tag(0),
m_count(0)
{
	init();
}

template<class QueueElmentType>
LockFreeQueue<QueueElmentType>::~LockFreeQueue()
{
	destroy();
	// 清理待删除列表中的节点
	for (auto node : m_retired)
	{
		delete node;
	}
}

template<class QueueElmentType>
void LockFreeQueue<QueueElmentType>::init()
{
	// 该函数假设调用时无并发（构造函数或 clear_unsafe 后调用）
	auto dummy = new QueueNode < QueueElmentType > ;
	dummy->m_next.store(nullptr, std::memory_order_seq_cst);

	VersionPtr<QueueElmentType> vp;
	vp.m_ptr = dummy;
	vp.m_version = 1;
	m_front.store(vp, std::memory_order_seq_cst);
	m_rear.store(vp, std::memory_order_seq_cst);
	m_tag.store(0, std::memory_order_seq_cst);
	m_count.store(0, std::memory_order_seq_cst);
}

template<class QueueElmentType>
void LockFreeQueue<QueueElmentType>::destroy()
{
	// 非线程安全，仅应在无并发时调用
	clear_unsafe();
	// 清理待删除列表（实际上 clear_unsafe 已处理所有节点，此处仅为防御）
	for (auto node : m_retired)
	{
		delete node;
	}
	m_retired.clear();
}

template<class QueueElmentType>
void LockFreeQueue<QueueElmentType>::retire_node(QueueNode<QueueElmentType>* node)
{
	// 仅由单消费者线程调用，无竞争
	m_retired.push_back(node);
}

template<class QueueElmentType>
void LockFreeQueue<QueueElmentType>::gc()
{
	// 仅由单消费者线程调用，释放所有待删除节点
	for (auto node : m_retired)
	{
		delete node;
	}
	m_retired.clear();
}

template<class QueueElmentType>
void LockFreeQueue<QueueElmentType>::push(const QueueElmentType& e)
{
	auto newNode = new QueueNode < QueueElmentType > ;
	newNode->m_data = e;
	newNode->m_next.store(nullptr, std::memory_order_seq_cst);

	while (true)
	{
		VersionPtr<QueueElmentType> tail = m_rear.load(std::memory_order_seq_cst);
		QueueNode<QueueElmentType>* next = tail.m_ptr->m_next.load(std::memory_order_seq_cst);

		if (tail != m_rear.load(std::memory_order_seq_cst))
		{
			continue;
		}

		if (next == nullptr)
		{
			if (tail.m_ptr->m_next.compare_exchange_weak(next, newNode, std::memory_order_seq_cst, std::memory_order_seq_cst))
			{
				// 链接成功，尝试推进 rear（即使失败也认为有其他线程会推进）
				size_t newVer = m_tag.fetch_add(1, std::memory_order_seq_cst) + 1;
				VersionPtr<QueueElmentType> newTail;
				newTail.m_ptr = newNode;
				newTail.m_version = newVer;
				m_rear.compare_exchange_weak(tail, newTail, std::memory_order_seq_cst, std::memory_order_seq_cst);
				m_count.fetch_add(1, std::memory_order_seq_cst);
				return;
			}
		}
		else
		{
			// 协助推进落后的 rear，并持续尝试直到 rear 更新或 next 变化
			size_t newVer = m_tag.fetch_add(1, std::memory_order_seq_cst) + 1;
			VersionPtr<QueueElmentType> newTail;
			newTail.m_ptr = next;
			newTail.m_version = newVer;
			m_rear.compare_exchange_weak(tail, newTail, std::memory_order_seq_cst, std::memory_order_seq_cst);
			// 不立即 continue，让循环再次检查，以便继续推进或插入
		}
	}
}

template<class QueueElmentType>
bool LockFreeQueue<QueueElmentType>::pop(QueueElmentType* e)
{
	while (true)
	{
		VersionPtr<QueueElmentType> head = m_front.load(std::memory_order_seq_cst);
		VersionPtr<QueueElmentType> tail = m_rear.load(std::memory_order_seq_cst);
		QueueNode<QueueElmentType>* next = head.m_ptr->m_next.load(std::memory_order_seq_cst);

		// 二次确认 head 未被修改（确保 next 的有效性）
		if (head != m_front.load(std::memory_order_seq_cst))
		{
			continue;
		}

		if (head.m_ptr == tail.m_ptr)
		{
			if (next == nullptr)
			{
				// 队列确实为空
				return false;
			}

			// 协助推进落后的 rear（仅尝试一次，避免复杂循环）
			size_t newVer = m_tag.fetch_add(1, std::memory_order_seq_cst) + 1;
			VersionPtr<QueueElmentType> newTail;
			newTail.m_ptr = next;
			newTail.m_version = newVer;
			m_rear.compare_exchange_weak(tail, newTail, std::memory_order_seq_cst, std::memory_order_seq_cst);
			// 无论是否成功，重新开始 pop 流程
			continue;
		}
		else
		{
			// head != tail，尝试出队
			if (next == nullptr)
			{
				// 防御
				continue;
			}

			size_t newVer = m_tag.fetch_add(1, std::memory_order_seq_cst) + 1;
			VersionPtr<QueueElmentType> newHead;
			newHead.m_ptr = next;
			newHead.m_version = newVer;

			if (m_front.compare_exchange_weak(head, newHead, std::memory_order_seq_cst, std::memory_order_seq_cst))
			{
				*e = next->m_data;
				m_count.fetch_sub(1, std::memory_order_seq_cst);

				if (head.m_version > 1)
				{
					retire_node(head.m_ptr);
				}
				if (m_retired.size() >= 100)
				{
					gc();
				}
				return true;
			}
			// CAS 失败，重试
		}
	}
}

template<class QueueElmentType>
bool LockFreeQueue<QueueElmentType>::empty() const
{
	VersionPtr<QueueElmentType> f = m_front.load(std::memory_order_seq_cst);
	VersionPtr<QueueElmentType> r = m_rear.load(std::memory_order_seq_cst);
	return f.m_ptr == r.m_ptr;
}

template<class QueueElmentType>
int32_t LockFreeQueue<QueueElmentType>::size() const
{
	return m_count.load(std::memory_order_seq_cst);
}

template<class QueueElmentType>
void LockFreeQueue<QueueElmentType>::clear()
{
	// 线程安全清空：反复 pop 直到空（适用于单消费者线程）
	QueueElmentType dummy;
	while (pop(&dummy))
	{
		// 丢弃数据
	}
	// 清空后主动回收内存
	gc();
}

template<class QueueElmentType>
void LockFreeQueue<QueueElmentType>::clear_unsafe()
{
	// 非线程安全：直接释放链表（仅应在无并发时调用）
	VersionPtr<QueueElmentType> front = m_front.exchange(VersionPtr<QueueElmentType>(), std::memory_order_seq_cst);
	VersionPtr<QueueElmentType> rear = m_rear.exchange(VersionPtr<QueueElmentType>(), std::memory_order_seq_cst);
	m_count.store(0, std::memory_order_seq_cst);
	m_tag.store(0, std::memory_order_seq_cst);

	QueueNode<QueueElmentType>* curr = front.m_ptr;
	while (curr)
	{
		QueueNode<QueueElmentType>* next = curr->m_next.load(std::memory_order_seq_cst);
		delete curr;
		curr = next;
	}
}

#endif // _LOCK_FREE_QUEUE_H__