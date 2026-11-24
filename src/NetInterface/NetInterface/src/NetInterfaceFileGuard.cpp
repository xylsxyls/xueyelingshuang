#include "NetInterfaceFileGuard.h"
#include "NetInterfaceMacro.h"

NetInterfaceFileGuard::NetInterfaceFileGuard(void* bytes, uint32_t size) :
m_bytes(static_cast<uint8_t*>(bytes)),
m_size(size)
{

}

NetInterfaceFileGuard::~NetInterfaceFileGuard()
{
    NetInterface::wipe(m_bytes, m_size);
}

bool NetInterfaceFileGuard::overlap(const void* left, size_t leftSize, const void* right, size_t rightSize)
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

bool NetInterfaceFileGuard::overlapsFrame(const NetInterfaceFrame& frame, const void* result, size_t size)
{
    return overlap(result, size, &frame, sizeof(frame)) || overlap(result, size, frame.m_payload, frame.m_payloadSize);
}

bool NetInterfaceFileGuard::prepareCount(const void* bytes, uint32_t size, uint32_t* count)
{
    if (count == nullptr || overlap(bytes, size, count, sizeof(*count))) { return false; }
    *count = 0;
    return true;
}

bool NetInterfaceFileGuard::overlapsFrameLength(const NetInterfaceFrame& frame, const uint8_t* output, uint32_t capacity,
    const uint32_t& length)
{
    return overlapsFrame(frame, &length, sizeof(length)) ||
        overlap(&length, sizeof(length), output, capacity);
}
