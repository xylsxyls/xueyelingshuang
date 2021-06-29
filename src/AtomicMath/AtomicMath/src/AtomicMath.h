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

	/** 自减
	@param [in] ptr 4字节内存地址，可以用于共享内存
	@return 返回自减之后的值
	*/
	static int32_t selfSubOne(int32_t* ptr);

	/** 加
	@param [in] ptr 4字节内存地址，可以用于共享内存
	@param [in] value 计算值
	@return 返回加之后的值
	*/
	static int32_t selfAdd(int32_t* ptr, int32_t value);

	/** 减
	@param [in] ptr 4字节内存地址，可以用于共享内存
	@param [in] value 计算值
	@return 返回减之后的值
	*/
	static int32_t selfSub(int32_t* ptr, int32_t value);
};