#include "LockFreeHashMapPerfMemory.h"
#include <algorithm>
#include <cstdlib>
#include <limits>
#include <new>

bool LockFreeHashMapPerfMemory::s_tracking = false;
size_t LockFreeHashMapPerfMemory::s_bytes = 0;
size_t LockFreeHashMapPerfMemory::s_peak = 0;
uint64_t LockFreeHashMapPerfMemory::s_allocations = 0;

/** 单线程专用申请计量，保留 malloc 的基础对齐，调用边界负责捕获失败
@param [in] size 请求字节数
@return 非空载荷，失败抛 bad_alloc
*/
void* operator new(size_t size)
{
    if (size > (std::numeric_limits<size_t>::max)() - 32)
    {
        throw std::bad_alloc();
    }
    unsigned char* raw = static_cast<unsigned char*>(std::malloc(size + 32));
    if (raw == nullptr)
    {
        throw std::bad_alloc();
    }
    size_t* header = reinterpret_cast<size_t*>(raw);
    header[0] = size;
    header[1] = LockFreeHashMapPerfMemory::s_tracking ? 1 : 0;
    if (header[1])
    {
        LockFreeHashMapPerfMemory::s_bytes += size;
        LockFreeHashMapPerfMemory::s_peak = (std::max)(LockFreeHashMapPerfMemory::s_peak, LockFreeHashMapPerfMemory::s_bytes);
        ++LockFreeHashMapPerfMemory::s_allocations;
    }
    return raw + 32;
}

/** 释放并撤销对应的计量
@param [in] pointer 本程序配对申请的载荷，可为空
*/
void operator delete(void* pointer) throw()
{
    if (pointer != nullptr)
    {
        unsigned char* raw = static_cast<unsigned char*>(pointer) - 32;
        size_t* header = reinterpret_cast<size_t*>(raw);
        if (header[1])
        {
            LockFreeHashMapPerfMemory::s_bytes -= header[0];
        }
        std::free(raw);
    }
}

/** 数组使用同一计量器
@param [in] size 包含实现所需数组开销的请求字节数
@return 配对载荷，失败抛 bad_alloc
*/
void* operator new[](size_t size)
{
    return ::operator new(size);
}

/** 释放配对数组申请
@param [in] pointer 可为空的载荷
*/
void operator delete[](void* pointer) throw()
{
    ::operator delete(pointer);
}