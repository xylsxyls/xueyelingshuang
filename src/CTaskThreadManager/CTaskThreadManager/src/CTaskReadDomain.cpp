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

#ifdef _MSC_VER
__declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
__attribute__((noinline))
#endif
CTaskReadPosition* CTaskReadDomain::enterFallback(size_t preferred, size_t& hint, bool* contended)
{
    if (m_closed.load(std::memory_order_seq_cst))
    {
        return nullptr;
    }
    for (size_t step = 1; step < m_count; ++step)
    {
        const size_t index = step < m_count - preferred ? preferred + step : step - (m_count - preferred);
        CTaskReadPosition* position = tryEnter(index, contended);
        if (position != nullptr)
        {
            hint = index;
            return position;
        }
    }
    hint = preferred + 1 == m_count ? 0 : preferred + 1;
    return nullptr;
}

void CTaskReadDomain::close()
{
    m_closed.store(true, std::memory_order_seq_cst);
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