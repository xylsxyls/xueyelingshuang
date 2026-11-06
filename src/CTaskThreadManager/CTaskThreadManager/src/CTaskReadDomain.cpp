#include "CTaskReadDomain.h"
#include <new>
#include <limits>

CTaskReadPosition::CTaskReadPosition() :
m_epoch(0)
{

}

CTaskReadDomain::CTaskReadDomain() :
m_storage(nullptr),
m_positions(nullptr),
m_count(0),
m_epoch(1),
m_closed(true)
{

}

CTaskReadDomain::~CTaskReadDomain()
{
    for (size_t i = 0; i < m_count; ++i)
    {
        m_positions[i].~CTaskReadPosition();
    }
    delete[] m_storage;
}

bool CTaskReadDomain::atomicsLockFree() const
{
    return m_epoch.is_lock_free() && m_closed.is_lock_free();
}

bool CTaskReadDomain::initialize(size_t count)
{
    if (m_storage != nullptr || count == 0 || count > ((std::numeric_limits<size_t>::max)() - 63) / sizeof(CTaskReadPosition) ||
        !atomicsLockFree())
    {
        return false;
    }
    try
    {
        unsigned char* storage = new unsigned char[count * sizeof(CTaskReadPosition) + 63];
        CTaskReadPosition* positions = reinterpret_cast<CTaskReadPosition*>((reinterpret_cast<uintptr_t>(storage) + 63) & ~uintptr_t(63));
        for (size_t i = 0; i < count; ++i)
        {
            new (&positions[i]) CTaskReadPosition();
        }
        m_storage = storage;
        m_positions = positions;
        m_count = count;
        m_closed.store(false, std::memory_order_release);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

CTaskReadPosition* CTaskReadDomain::enter()
{
    if (m_closed.load(std::memory_order_seq_cst))
    {
        return nullptr;
    }
#if defined(_MSC_VER) && _MSC_VER == 1800
    static __declspec(thread) size_t s_hint = 0;
#else
    static thread_local size_t s_hint = 0;
#endif
    const size_t preferred = s_hint < m_count ? s_hint : 0;
    for (size_t step = 0; step < m_count; ++step)
    {
        const size_t index = step == 0 ? preferred : (step <= preferred ? step - 1 : step);
        CTaskReadPosition* position = tryEnter(index);
        if (position != nullptr)
        {
            s_hint = index;
            return position;
        }
    }
    return nullptr;
}

CTaskReadPosition* CTaskReadDomain::tryEnter(size_t index)
{
    if (index >= m_count || m_closed.load(std::memory_order_seq_cst))
    {
        return nullptr;
    }
    CTaskReadPosition& position = m_positions[index];
    uint64_t expected = 0;
    uint64_t epoch = m_epoch.load(std::memory_order_seq_cst);
    if (!position.m_epoch.compare_exchange_strong(expected, epoch, std::memory_order_seq_cst, std::memory_order_relaxed))
    {
        return nullptr;
    }
    // 公布与验证都早于受保护地址的首次解引用；暂停借用者只延迟回收。
    while (epoch != m_epoch.load(std::memory_order_seq_cst))
    {
        epoch = m_epoch.load(std::memory_order_seq_cst);
        position.m_epoch.store(epoch, std::memory_order_seq_cst);
    }
    if (m_closed.load(std::memory_order_seq_cst))
    {
        position.m_epoch.store(0, std::memory_order_release);
        return nullptr;
    }
    return &position;
}

size_t CTaskReadDomain::indexOf(const CTaskReadPosition& position) const
{
    return static_cast<size_t>(&position - m_positions);
}

void CTaskReadDomain::leave(CTaskReadPosition& position)
{
    position.m_epoch.store(0, std::memory_order_release);
}

void CTaskReadDomain::close()
{
    m_closed.store(true, std::memory_order_seq_cst);
}

bool CTaskReadDomain::closed() const
{
    return m_closed.load(std::memory_order_acquire);
}

size_t CTaskReadDomain::active() const
{
    size_t count = 0;
    for (size_t i = 0; i < m_count; ++i)
    {
        count += m_positions[i].m_epoch.load(std::memory_order_seq_cst) != 0 ? 1 : 0;
    }
    return count;
}

uint64_t CTaskReadDomain::advance()
{
    const uint64_t previous = m_epoch.load(std::memory_order_relaxed);
    if (previous == UINT64_MAX)
    {
        return 0;
    }
    m_epoch.store(previous + 1, std::memory_order_seq_cst);
    return previous;
}

uint64_t CTaskReadDomain::oldest() const
{
    uint64_t oldest = UINT64_MAX;
    for (size_t i = 0; i < m_count; ++i)
    {
        const uint64_t epoch = m_positions[i].m_epoch.load(std::memory_order_seq_cst);
        if (epoch != 0 && epoch < oldest)
        {
            oldest = epoch;
        }
    }
    return oldest;
}

uint64_t CTaskReadDomain::current() const
{
    return m_epoch.load(std::memory_order_acquire);
}

size_t CTaskReadDomain::capacity() const
{
    return m_count;
}