#pragma once
#include "AtomicMathMacro.h"
#include <stdint.h>

/** 原子计算类
*/
class AtomicMathAPI AtomicMath
{
public:
	/** 自加
	@param [in] ptr 4字节内存地址，可以用于共享内存
	@return 返回自加之后的值
	*/
	static int32_t selfAddOne(int32_t* ptr);

	/** 自加
	@param [in] ptr 8字节内存地址，可以用于共享内存
	@return 返回自加之后的值
	*/
	static int64_t selfAddOne(int64_t* ptr);

	/** 自减
	@param [in] ptr 4字节内存地址，可以用于共享内存
	@return 返回自减之后的值
	*/
	static int32_t selfSubOne(int32_t* ptr);

	/** 自减
	@param [in] ptr 8字节内存地址，可以用于共享内存
	@return 返回自减之后的值
	*/
	static int64_t selfSubOne(int64_t* ptr);

	/** 加
	@param [in] ptr 4字节内存地址，可以用于共享内存
	@param [in] value 计算值
	@return 返回加之后的值
	*/
	static int32_t selfAdd(int32_t* ptr, int32_t value);

	/** 加
	@param [in] ptr 8字节内存地址，可以用于共享内存
	@param [in] value 计算值
	@return 返回加之后的值
	*/
	static int64_t selfAdd(int64_t* ptr, int64_t value);

	/** 减
	@param [in] ptr 4字节内存地址，可以用于共享内存
	@param [in] value 计算值
	@return 返回减之后的值
	*/
	static int32_t selfSub(int32_t* ptr, int32_t value);

	/** 减
	@param [in] ptr 8字节内存地址，可以用于共享内存
	@param [in] value 计算值
	@return 返回减之后的值
	*/
	static int64_t selfSub(int64_t* ptr, int64_t value);

	/** 比较之后交换
	@param [in] ptr 4字节内存地址，可以用于共享内存
	@param [in] oldValue 是否和内存中原本的值一致
	@param [in] newValue 如果一致则修改为新的值
	@return 成功更换值返回true
	*/
	static bool compareAndSwap(int32_t* ptr, int32_t oldValue, int32_t newValue);

	/** 比较之后交换
	@param [in] ptr 8字节内存地址，可以用于共享内存
	@param [in] oldValue 是否和内存中原本的值一致
	@param [in] newValue 如果一致则修改为新的值
	@return 成功更换值返回true
	*/
	static bool compareAndSwap(int64_t* ptr, int64_t oldValue, int64_t newValue);
};