#pragma once
#include <cstddef>
#include <new>

#ifndef LOCK_FREE_MAP_BENCHMARK_PLAIN
/** 申请带对齐头的测试内存并记录字节数
@param [in] size 申请字节数
@return 有效载荷地址，失败抛出bad_alloc
*/
void* operator new(size_t size);

/** 归还测试内存并减少对应字节数
@param [in] memory 原申请返回的地址，允许为空
*/
void operator delete(void* memory) throw();

/** 数组申请使用同一分配仪表
@param [in] size 数组所需的完整申请字节数
@return 有效载荷地址，失败抛出bad_alloc
*/
void* operator new[](size_t size);

/** 归还数组使用的测试内存
@param [in] memory 分配器返回的数组申请地址，允许为空
*/
void operator delete[](void* memory) throw();

/** 申请失败返回空指针，仍跟踪成功申请的字节数
@param [in] size 申请字节数
@param [in] tag 标准非抛出标记
@return 有效载荷地址或空指针
*/
void* operator new(size_t size, const std::nothrow_t& tag) throw();

/** 非抛出数组申请使用同一分配仪表
@param [in] size 申请字节数
@param [in] tag 标准非抛出标记
@return 有效载荷地址或空指针
*/
void* operator new[](size_t size, const std::nothrow_t& tag) throw();

/** 非抛出申请后构造异常时的配对释放
@param [in] memory 原申请地址，允许为空
@param [in] tag 标准非抛出标记
*/
void operator delete(void* memory, const std::nothrow_t& tag) throw();

/** 非抛出数组构造异常时的配对释放
@param [in] memory 原数组申请地址，允许为空
@param [in] tag 标准非抛出标记
*/
void operator delete[](void* memory, const std::nothrow_t& tag) throw();
#endif