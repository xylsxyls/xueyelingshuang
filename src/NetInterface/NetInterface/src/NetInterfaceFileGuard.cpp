#include "NetInterfaceFileGuard.h"
#include "NetInterfaceMacro.h"
#include "NetInterfaceFile.h"
#include <cstring>

NetInterfaceFileGuard::NetInterfaceFileGuard(void* bytes, uint32_t size) :
m_bytes(static_cast<uint8_t*>(bytes)),
m_size(size)
{

}

NetInterfaceFileGuard::~NetInterfaceFileGuard()
{
    NetInterface::wipe(m_bytes, m_size);
}

bool NetInterfaceFileGuard::overlap(const void* left, uint32_t leftSize, const void* right, uint32_t rightSize)
{
    if (left == nullptr || right == nullptr || leftSize == 0 || rightSize == 0)
    {
        return false;
    }
    const uintptr_t first = reinterpret_cast<uintptr_t>(left);
    const uintptr_t second = reinterpret_cast<uintptr_t>(right);
    return first > UINTPTR_MAX - leftSize || second > UINTPTR_MAX - rightSize ||
        (first < second + rightSize && second < first + leftSize);
}