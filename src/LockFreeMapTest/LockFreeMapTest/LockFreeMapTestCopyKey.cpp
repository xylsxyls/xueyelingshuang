#include "LockFreeMapTestCopyKey.h"
#include <stdexcept>

std::atomic<int> LockFreeMapTestCopyKey::m_copies(0);
bool LockFreeMapTestCopyKey::m_throw = false;

LockFreeMapTestCopyKey::LockFreeMapTestCopyKey(int value) :
m_value(value)
{

}

LockFreeMapTestCopyKey::LockFreeMapTestCopyKey(const LockFreeMapTestCopyKey& other) :
m_value(other.m_value)
{
    m_copies.fetch_add(1);
    if (m_throw)
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