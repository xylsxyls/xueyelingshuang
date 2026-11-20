#include "LockFreeMapTestCopyKey.h"
#include <stdexcept>

std::atomic<int> LockFreeMapTestCopyKey::m_copies(0);
int LockFreeMapTestCopyKey::m_failOnCopy = 0;

LockFreeMapTestCopyKey::LockFreeMapTestCopyKey(int value) :
m_value(value)
{

}

LockFreeMapTestCopyKey::LockFreeMapTestCopyKey(const LockFreeMapTestCopyKey& other) :
m_value(other.m_value)
{
    if (m_copies.fetch_add(1) + 1 == m_failOnCopy)
    {
        throw std::runtime_error("injected key copy");
    }
}

bool LockFreeMapTestCopyKey::operator<(const LockFreeMapTestCopyKey& other) const
{
    return m_value < other.m_value;
}

LockFreeMapTestCopyKey& LockFreeMapTestCopyKey::operator=(const LockFreeMapTestCopyKey& other)
{
    m_value = other.m_value;
    return *this;
}