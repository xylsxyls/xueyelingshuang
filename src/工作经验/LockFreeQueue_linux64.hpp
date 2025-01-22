#pragma once
#include <stdint.h>
#include <atomic>

template<class QueueElmentType>
struct QueueNode;

template<class QueueElmentType>
struct VersionPtr
{
    VersionPtr():
    m_ptr(nullptr),
    m_version(0)
    {

    }

    bool empty()
    {
        return m_ptr == nullptr && m_version == 0;
    }

    bool operator==(const VersionPtr& other) const
    {
        return m_ptr == other.m_ptr && m_version == other.m_version;
    }

    bool operator!=(const VersionPtr& other) const
    {
        return m_ptr != other.m_ptr || m_version != other.m_version;
    }

    QueueNode<QueueElmentType>* m_ptr;
    size_t m_version;
};

template<class QueueElmentType>
struct QueueNode
{
    QueueNode()
    {

    }

    std::atomic<VersionPtr<QueueElmentType>> m_next;
    QueueElmentType m_data;
};

template<class QueueElmentType>
class LockFreeQueue
{
public:
    LockFreeQueue():
    m_count(0),
    m_tag(1)
    {
        init();
    }

    ~LockFreeQueue()
    {
        destroy();
    }

public:
    void init()
    {
        if (!m_rear.load().empty())
        {
            return;
        }
        VersionPtr<QueueElmentType> p;
        p.m_ptr = new QueueNode<QueueElmentType>;
        p.m_version = 1;
        m_rear.store(p);
        m_front = m_rear.load();
    }

    void clear()
    {
        while (m_front.load() != m_rear.load())
        {
            std::atomic<VersionPtr<QueueElmentType>> tmp(m_front.load().m_ptr->m_next.load());
            delete m_front.load().m_ptr;
            m_front = tmp.load();
            --m_count;
        }
    }

    void destroy()
    {
        if (m_rear.load().empty())
        {
            return;
        }
        clear();
        delete m_front.load().m_ptr;
        m_rear.store(VersionPtr<QueueElmentType>());
        m_front.store(m_rear.load());
    }

    void push(const QueueElmentType& e)
    {
        VersionPtr<QueueElmentType> p;
        p.m_ptr = new QueueNode<QueueElmentType>;
        p.m_version = m_tag.fetch_add(1);
        while (true)
        {
            VersionPtr<QueueElmentType> nullexpected;
            if (__atomic_compare_exchange(reinterpret_cast<VersionPtr<QueueElmentType>*>(&(m_rear.load().m_ptr->m_next)),
                &nullexpected, &p, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
            {
                m_rear.load().m_ptr->m_data = e;
                ++m_count;
                m_rear = p;
                break;
            }
        }
    }

    bool pop(QueueElmentType* e)
    {
        while (true)
        {
            if (empty())
            {
                return false;
            }
            VersionPtr<QueueElmentType> p = m_front.load();
            VersionPtr<QueueElmentType> next = ((reinterpret_cast<VersionPtr<QueueElmentType>*>(&m_front))->m_ptr)->m_next.load();
            if (__atomic_compare_exchange(reinterpret_cast<VersionPtr<QueueElmentType>*>(&m_front),
                &p, &next, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
            {
                --m_count;
                *e = p.m_ptr->m_data;
                delete p.m_ptr;
                break;
            }
        }

        return true;
    }

    bool empty()
    {
        return m_front.load() == m_rear.load();
    }

    int32_t size()
    {
        return m_count;
    }

private:
    std::atomic<VersionPtr<QueueElmentType>> m_front;
    std::atomic<VersionPtr<QueueElmentType>> m_rear;
    std::atomic<int32_t> m_count;
    std::atomic<size_t> m_tag;
};