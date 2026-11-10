#include "CTaskFaultAllocation.h"
#include <new>
#include <cstdlib>

#ifdef _MSC_VER
__declspec(thread) static uint64_t s_faultPosition = 0;
__declspec(thread) static uint64_t s_allocationCount = 0;
__declspec(thread) static bool s_faultTriggered = false;
#else
static thread_local uint64_t s_faultPosition = 0;
static thread_local uint64_t s_allocationCount = 0;
static thread_local bool s_faultTriggered = false;
#endif

void CTaskFaultAllocation::begin(uint64_t nth)
{
    s_faultPosition = nth;
    s_allocationCount = 0;
    s_faultTriggered = false;
}

uint64_t CTaskFaultAllocation::end()
{
    s_faultPosition = 0;
    return s_allocationCount;
}

bool CTaskFaultAllocation::triggered()
{
    return s_faultTriggered;
}

void* operator new(size_t size)
{
    if (s_faultPosition != 0 && ++s_allocationCount == s_faultPosition)
    {
        s_faultPosition = 0;
        s_faultTriggered = true;
        throw std::bad_alloc();
    }
    void* memory = std::malloc(size == 0 ? 1 : size);
    if (memory == nullptr)
    {
        throw std::bad_alloc();
    }
    return memory;
}

void operator delete(void* memory) throw()
{
    std::free(memory);
}

void* operator new[](size_t size)
{
    return ::operator new(size);
}

void operator delete[](void* memory) throw()
{
    ::operator delete(memory);
}