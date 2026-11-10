#ifndef LOCK_FREE_QUEUE_LIST_MUTEX_INL
#define LOCK_FREE_QUEUE_LIST_MUTEX_INL

template<class T>
LockFreeQueue<T>::LockFreeQueue()
{
}

template<class T>
LockFreeQueue<T>::~LockFreeQueue()
{
}

template<class T>
void LockFreeQueue<T>::init()
{
    clear();
}

template<class T>
void LockFreeQueue<T>::destroy()
{
    clear();
}

template<class T>
void LockFreeQueue<T>::clear()
{
    std::lock_guard<std::mutex> guard(m_mutex);
    m_items.clear();
}

template<class T>
void LockFreeQueue<T>::push(const T& value)
{
    std::lock_guard<std::mutex> guard(m_mutex);
    m_items.push_back(value);
}

template<class T>
void LockFreeQueue<T>::assign(T& output, T& source, std::true_type)
{
    output = std::move(source);
}

template<class T>
void LockFreeQueue<T>::assign(T& output, T& source, std::false_type)
{
    output = static_cast<const T&>(source);
}

template<class T>
bool LockFreeQueue<T>::pop(T* output)
{
    std::lock_guard<std::mutex> guard(m_mutex);
    if (m_items.empty())
    {
        return false;
    }
    if (output != nullptr)
    {
        assign(*output, m_items.front(), typename std::is_nothrow_move_assignable<T>::type());
    }
    m_items.pop_front();
    return true;
}

template<class T>
bool LockFreeQueue<T>::empty()
{
    std::lock_guard<std::mutex> guard(m_mutex);
    return m_items.empty();
}

template<class T>
uint64_t LockFreeQueue<T>::size()
{
    std::lock_guard<std::mutex> guard(m_mutex);
    return static_cast<uint64_t>(m_items.size());
}

#endif