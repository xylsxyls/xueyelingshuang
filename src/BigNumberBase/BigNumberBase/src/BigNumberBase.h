#pragma once
#include "BigNumberBaseMacro.h"
#include <stdint.h>

class BigNumberBaseAPI BigNumberBase
{
public:
	enum BigNumberCompare
	{
		BIG,
		SMALL,
		EQUAL
	};
public:
	/** 构造函数
	*/
	BigNumberBase();

	/** 构造函数
	@param [in] num 数值
	*/
	BigNumberBase(int32_t num);

	/** 构造函数
	@param [in] num 数值字符串
	*/
	BigNumberBase(const char* num);

public:
	friend BigNumberBase operator + (const BigNumberBase& x, const BigNumberBase& y);
	friend BigNumberBase operator - (const BigNumberBase& x, const BigNumberBase& y);
	friend BigNumberBase operator * (const BigNumberBase& x, const BigNumberBase& y);
	friend BigNumberBase operator / (const BigNumberBase& x, const BigNumberBase& y);
	friend BigNumberBase operator % (const BigNumberBase& x, const BigNumberBase& y);

	BigNumberCompare Compare(const BigNumberBase& x, const BigNumberBase& y);
};