#include "LockFreeMapBenchmarkAllocator.h"
#include "LockFreeMapBenchmark.h"
#include <cstdlib>
#include <cstddef>
#include <type_traits>

// 分配仪表单独编译并关闭跨模块优化，避免同一编译单元内的分配改写破坏统计
#ifndef LOCK_FREE_MAP_BENCHMARK_PLAIN
/** 测试申请跟踪，包含索引数组和固定元数据
*/
void* operator new(size_t size)
{
    const size_t header = sizeof(std::aligned_storage<2 * sizeof(size_t), std::alignment_of<std::max_align_t>::value>::type);
    if (size > (std::numeric_limits<size_t>::max)() - header)
    {
        throw std::bad_alloc();
    }
    unsigned char* block = static_cast<unsigned char*>(std::malloc((size == 0 ? 1 : size) + header));
    if (block == nullptr)
    {
        throw std::bad_alloc();
    }
    // 在原始存储中显式开始计数字段的生命周期，保持 C++11 对象模型要求
    ::new (block) size_t(size);
    const size_t bytes = LockFreeMapBenchmark::m_bytes.fetch_add(size) + size;
    size_t peak = LockFreeMapBenchmark::m_peak.load();
    while (bytes > peak && !LockFreeMapBenchmark::m_peak.compare_exchange_weak(peak, bytes))
    {
    }
    LockFreeMapBenchmark::m_newCalls.fetch_add(1);
    return block + header;
}

void operator delete(void* memory) throw()
{
    if (memory != nullptr)
    {
        const size_t header = sizeof(std::aligned_storage<2 * sizeof(size_t), std::alignment_of<std::max_align_t>::value>::type);
        unsigned char* block = static_cast<unsigned char*>(memory) - header;
        LockFreeMapBenchmark::m_bytes.fetch_sub(*reinterpret_cast<size_t*>(block));
        std::free(block);
    }
}

void* operator new[](size_t size)
{
    return ::operator new(size);
}

void operator delete[](void* memory) throw()
{
    ::operator delete(memory);
}

/** 使非抛出申请也使用同一个对齐头和计数器，避免与运行库分配器混用
@param [in] size 申请字节数
@return 失败时返回空指针
*/
void* operator new(size_t size, const std::nothrow_t&) throw()
{
    try
    {
        return ::operator new(size);
    }
    catch (...)
    {
        return nullptr;
    }
}

void* operator new[](size_t size, const std::nothrow_t& tag) throw()
{
    return ::operator new(size, tag);
}

void operator delete(void* memory, const std::nothrow_t&) throw()
{
    ::operator delete(memory);
}

void operator delete[](void* memory, const std::nothrow_t&) throw()
{
    ::operator delete(memory);
}

#endif