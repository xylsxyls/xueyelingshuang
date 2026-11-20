#pragma once
#include <new>

// 每个使用全局分配计账的测试模块只包含一次，保证普通和非抛出分配都携带同一计账头。
/** 非抛异常申请捕获故障，不把空指针伪装成成功
@param [in] size 字节数
@param [in] tag 非抛异常重载标签
@return 地址或空指针
*/
void* operator new(size_t size, const std::nothrow_t& tag) throw()
{
    (void)tag;
    try
    {
        return ::operator new(size);
    }
    catch (...)
    {
        return nullptr;
    }
}

/** 非抛异常数组申请使用相同计账
@param [in] size 字节数
@param [in] tag 非抛异常重载标签
@return 地址或空指针
*/
void* operator new[](size_t size, const std::nothrow_t& tag) throw()
{
    return ::operator new(size, tag);
}

/** 非抛异常构造失败的释放重载
@param [in] pointer 已申请地址
@param [in] tag 非抛异常重载标签
*/
void operator delete(void* pointer, const std::nothrow_t& tag) throw()
{
    (void)tag;
    ::operator delete(pointer);
}

/** 非抛异常数组构造失败的释放重载
@param [in] pointer 已申请地址
@param [in] tag 非抛异常重载标签
*/
void operator delete[](void* pointer, const std::nothrow_t& tag) throw()
{
    (void)tag;
    ::operator delete(pointer);
}