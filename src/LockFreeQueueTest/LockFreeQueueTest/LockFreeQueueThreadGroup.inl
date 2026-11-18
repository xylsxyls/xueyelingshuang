inline LockFreeQueueThreadGroup::LockFreeQueueThreadGroup(std::atomic<bool>& stop, std::atomic<bool>* start) :
m_stop(&stop), m_start(start)
{

}

inline LockFreeQueueThreadGroup::~LockFreeQueueThreadGroup()
{
    m_stop->store(true);
    if (m_start != nullptr)
    {
        m_start->store(true);
    }
    join();
}

inline void LockFreeQueueThreadGroup::join()
{
    for (size_t i = 0; i < m_threads.size(); ++i)
    {
        if (m_threads[i].joinable())
        {
            m_threads[i].join();
        }
    }
}

template <class Queue>
void LockFreeQueueThreadGroup::cancelAndDrain(Queue& queue, const std::atomic<size_t>& completed)
{
    m_stop->store(true);
    if (m_start != nullptr)
    {
        m_start->store(true);
    }
    while (completed.load() != m_threads.size())
    {
        if (!queue.pop(nullptr))
        {
            std::this_thread::yield();
        }
    }
    join();
    queue.clear();
}