#ifndef __LOCK_FREE_QUEUE_INL__
#define __LOCK_FREE_QUEUE_INL__

// ==================== QueueNode 实现 ====================

template<class QueueElementType>
QueueNode<QueueElementType>::QueueNode() :
m_next(nullptr),
m_hasData(false)
{
}

template<class QueueElementType>
QueueNode<QueueElementType>::QueueNode(const QueueElementType& data) :
m_next(nullptr),
m_hasData(false)
{
	construct_data(data);
}

template<class QueueElementType>
QueueNode<QueueElementType>::~QueueNode()
{
	destroy_data();
}

template<class QueueElementType>
QueueElementType* QueueNode<QueueElementType>::data_ptr()
{
	return reinterpret_cast<QueueElementType*>(&m_storage);
}

template<class QueueElementType>
const QueueElementType* QueueNode<QueueElementType>::data_ptr() const
{
	return reinterpret_cast<const QueueElementType*>(&m_storage);
}

template<class QueueElementType>
void QueueNode<QueueElementType>::construct_data(const QueueElementType& data)
{
	new (&m_storage) QueueElementType(data);
	m_hasData = true;
}

template<class QueueElementType>
void QueueNode<QueueElementType>::destroy_data()
{
	if (m_hasData)
	{
		data_ptr()->~QueueElementType();
		m_hasData = false;
	}
}

// ==================== LockFreeQueue 实现 ====================

template<class QueueElementType>
LockFreeQueue<QueueElementType>::LockFreeQueue() :
m_head(nullptr),
m_tail(nullptr),
m_count(0)
{
	init();
}

template<class QueueElementType>
LockFreeQueue<QueueElementType>::~LockFreeQueue()
{
	destroy();
}

template<class QueueElementType>
void LockFreeQueue<QueueElementType>::init()
{
	/*
	init 必须无并发调用。
	支持：
	1. 构造函数首次 init；
	2. destroy 后再次 init；
	3. 队列已有内容时重新 init。
	*/
	destroy();

	QueueNode<QueueElementType>* dummy = new QueueNode<QueueElementType>();

	m_head = dummy;
	m_tail.store(dummy, std::memory_order_release);
	m_count.store(0, std::memory_order_relaxed);
}

template<class QueueElementType>
void LockFreeQueue<QueueElementType>::destroy()
{
	/*
	destroy 必须无并发调用。
	destroy 时队列里可以有内容。
	这里先 clear 掉所有真实数据节点，然后删除最后剩下的哨兵节点。
	destroy 后：
	m_head == nullptr
	m_tail == nullptr
	m_count == 0
	之后可以再次 init。
	*/

	if (m_head == nullptr)
	{
		m_tail.store(nullptr, std::memory_order_release);
		m_count.store(0, std::memory_order_relaxed);
		return;
	}

	// 无并发场景下，clear 会弹出所有真实元素，只剩最后一个哨兵节点。
	clear();

	QueueNode<QueueElementType>* dummy = m_head;
	m_head = nullptr;

	m_tail.store(nullptr, std::memory_order_release);
	m_count.store(0, std::memory_order_relaxed);

	delete dummy;
}

template<class QueueElementType>
void LockFreeQueue<QueueElementType>::push(const QueueElementType& e)
{
	/*
	这里只 new 一次 QueueNode。
	QueueElementType 通过 placement new 直接构造在 QueueNode 内部。
	没有额外：
	new QueueElementType(e)
	但由于接口是 push(const T&)，这里仍然会调用一次 T 的拷贝构造。
	*/
	QueueNode<QueueElementType>* newNode = new QueueNode<QueueElementType>(e);

	/*
	count 提前增加。
	这样即使生产者已经进入 push，但还没来得及 exchange/link，
	empty()/size() 也会保守地认为队列可能非空。
	count 不参与数据同步，只做近似统计，所以 relaxed 足够。
	*/
	m_count.fetch_add(1, std::memory_order_relaxed);

	/*
	MPSC 多生产者核心：
	prev = tail.exchange(newNode)
	prev->next = newNode
	exchange 不依赖 expected 指针判断，因此这里不需要 VersionPtr。
	*/
	QueueNode<QueueElementType>* prev = m_tail.exchange(newNode, std::memory_order_acq_rel);

	assert(prev != nullptr && "LockFreeQueue::push called before init() or after destroy().");

	if (prev == nullptr)
	{
		/*
		理论上只有违反 init/destroy 无并发约束，或者 destroy 后未重新 init 又 push，
		才可能发生。
		这里做 release 版本防御，避免泄漏 newNode。
		*/
		m_count.fetch_sub(1, std::memory_order_relaxed);
		m_tail.store(nullptr, std::memory_order_release);
		delete newNode;
		return;
	}

	/*
	发布链表链接。
	consumer acquire 读取到 newNode 后，
	能看到 QueueNode 构造完成以及其中的 QueueElementType 数据。
	*/
	prev->m_next.store(newNode, std::memory_order_release);
}

template<class QueueElementType>
bool LockFreeQueue<QueueElementType>::pop(QueueElementType* e)
{
	/*
	只能由单消费者执行流调用。

	e != nullptr：弹出并返回数据。
	e == nullptr：弹出并丢弃数据。

	clear() 内部也调用 pop(nullptr)，
	所以 clear 和 pop 必须串行。
	*/

	while (true)
	{
		QueueNode<QueueElementType>* head = m_head;

		if (head == nullptr)
		{
			return false;
		}

		QueueNode<QueueElementType>* next =
			head->m_next.load(std::memory_order_acquire);

		if (next != nullptr)
		{
			/*
			next 是真实数据节点。
			pop 成功后，next 会变成新的哨兵节点。
			所以：
			1. 如果 e != nullptr，先把数据交给调用者；
			2. 再析构 next 内部数据；
			3. next 作为无数据哨兵继续存在；
			4. 删除旧哨兵 head。
			*/
			if (e != nullptr)
			{
				assert(next->m_hasData && "LockFreeQueue internal error: data node has no data.");
				/*
				C++11 下 std::move 可用。
				对 shared_ptr / string / vector / 大对象：
				这里会优先走移动赋值；
				如果类型没有移动赋值但支持拷贝赋值，也会退化为拷贝赋值。
				*/
				*e = std::move(*(next->data_ptr()));
			}

			next->destroy_data();
			/*
			推进消费者侧 head。
			单消费者模型下，只有一个执行流写 m_head，
			所以不需要 CAS。
			*/
			m_head = next;

			m_count.fetch_sub(1, std::memory_order_relaxed);

			/*
			删除旧哨兵节点。
			为什么安全：
			只有看到 head->m_next != nullptr 后才删除 head。
			这说明负责链接 head 的生产者已经完成：
			prev->m_next.store(newNode)
			因此生产者不会再访问这个旧 head。
			*/
			delete head;

			return true;
		}

		QueueNode<QueueElementType>* tail = m_tail.load(std::memory_order_acquire);

		if (head == tail)
		{
			/*
			稳定空队列：
			head == tail
			head->next == nullptr
			*/
			return false;
		}

		/*
		MPSC exchange 队列的中间态：
		某个生产者已经执行：
		m_tail.exchange(newNode)
		但还没执行：
		prev->m_next.store(newNode)
		此时 head != tail，但 head->next 仍然是 nullptr。
		consumer 不能认为队列为空，也不能删除 head。
		*/
		std::this_thread::yield();
	}
}

template<class QueueElementType>
void LockFreeQueue<QueueElementType>::clear()
{
	/*
	clear 可以和 push 并发。
	但 clear 属于消费者侧操作：
	不能和 pop 并发；
	不能多个 clear 并发。
	如果生产者持续不断 push，clear 可能会持续消费新进入的元素，
	直到某一刻观察到队列为空才返回。
	*/
	while (pop(nullptr));
}

template<class QueueElementType>
bool LockFreeQueue<QueueElementType>::empty() const
{
	/*
	不通过 m_head->m_next 判断。
	因为 empty() 可能被其他线程调用，
	而消费者线程可能正在 pop/clear 并删除旧 head。
	如果这里解引用 m_head，可能读到已释放节点。
	所以 empty() 只基于 m_count。
	*/
	return m_count.load(std::memory_order_relaxed) <= 0;
}

template<class QueueElementType>
int32_t LockFreeQueue<QueueElementType>::size() const
{
	return m_count.load(std::memory_order_relaxed);
}

#endif // __LOCK_FREE_QUEUE_INL__