inline LockFreeQueueThreadGroup::LockFreeQueueThreadGroup(std::atomic<bool>& stop, std::atomic<bool>* start) :
m_stop(stop), m_start(start)
{

}

inline LockFreeQueueThreadGroup::~LockFreeQueueThreadGroup()
{
    m_stop.store(true);
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