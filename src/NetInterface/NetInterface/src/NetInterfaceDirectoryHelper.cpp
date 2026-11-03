#include "NetInterfaceDirectoryHelper.h"

bool NetInterfaceDirectoryHelper::directoryRangesOverlap(const void* first, uint32_t firstSize, const void* second, uint32_t secondSize)
{
    if (first == nullptr || second == nullptr || firstSize == 0 || secondSize == 0)
    {
        return false;
    }
    const uintptr_t left = reinterpret_cast<uintptr_t>(first);
    const uintptr_t right = reinterpret_cast<uintptr_t>(second);
    return left <= right ? right - left < firstSize : left - right < secondSize;
}