#include "CTaskFaultAllocation.h"
#include <new>
#include <cstdlib>

#ifdef _MSC_VER
__declspec(thread) static uint64_t s_faultPosition = 0;
__declspec(thread) static uint64_t s_allocationCount = 0;
__declspec(thread) static bool s_faultTriggered = false;
__declspec(thread) static bool s_persistentFault = false;
#else
static thread_local uint64_t s_faultPosition = 0;
static thread_local uint64_t s_allocationCount = 0;
static thread_local bool s_faultTriggered = false;
static thread_local bool s_persistentFault = false;
#endif

void CTaskFaultAllocation::begin(uint64_t nth)
{
    s_faultPosition = nth;
    s_allocationCount = 0;
    s_faultTriggered = false;
    s_persistentFault = false;
}

void CTaskFaultAllocation::beginPersistent(uint64_t nth)
{
    begin(nth);
    s_persistentFault = true;
}

uint64_t CTaskFaultAllocation::end()
{
    s_faultPosition = 0;
    s_persistentFault = false;
    return s_allocationCount;
}

bool CTaskFaultAllocation::triggered()
{
    return s_faultTriggered;
}

/** 测试进程的标量分配入口，按当前线程指定位置和持续模式注入失败
@param [in] size 请求字节数，零按一字节分配
@return 分配地址；故障注入或系统分配失败按标准 new 契约抛 bad_alloc
*/
void* operator new(size_t size)
{
    if (s_faultPosition != 0 && ++s_allocationCount >= s_faultPosition)
    {
        if (!s_persistentFault)
        {
            s_faultPosition = 0;
        }
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

/** 归还测试进程通过替换 new 取得的存储
@param [in] memory 分配地址，可为空
*/
void operator delete(void* memory) throw()
{
    std::free(memory);
}

/** 数组分配复用标量分配的故障计数和持续失败模式
@param [in] size 编译器给出的完整存储字节数
@return 分配地址，失败抛 bad_alloc
*/
void* operator new[](size_t size)
{
    return ::operator new(size);
}

/** 归还数组分配的原始存储
@param [in] memory 数组分配入口返回的地址，可为空
*/
void operator delete[](void* memory) throw()
{
    ::operator delete(memory);
}