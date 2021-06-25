#pragma once
#include "AtomicMathMacro.h"
#include <stdint.h>

/** 原子计算类
*/
class AtomicMathAPI AtomicMath
{
public:
	/** 自加
	@param [in] 4字节内存地址，可以用于共享内存
	@return 返回自加之后的值
	*/
	static int32_t selfAddOne(int32_t* ptr);

	/** 自减
	@param [in] 4字节内存地址，可以用于共享内存
	@return 返回自减之后的值
	*/
	static int32_t selfSubOne(int32_t* ptr);
};