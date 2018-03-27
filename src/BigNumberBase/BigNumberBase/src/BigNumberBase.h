#pragma once
#include "BigNumberBaseMacro.h"
#include <stdint.h>
#include "gmp.h"
#include <string>

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

	/** 析构函数
	*/
	~BigNumberBase();

	/** 构造函数
	@param [in] num 数值
	*/
	BigNumberBase(int32_t num);

	/** 构造函数
	@param [in] num 数值字符串
	*/
	BigNumberBase(const char* num);

	/** 拷贝构造函数
	*/
	BigNumberBase(const BigNumberBase& num);

public:
	friend BigNumberBase operator + (const BigNumberBase& x, const BigNumberBase& y);
	friend BigNumberBase operator - (const BigNumberBase& x, const BigNumberBase& y);
	friend BigNumberBase operator * (const BigNumberBase& x, const BigNumberBase& y);
	friend BigNumberBase operator / (const BigNumberBase& x, const BigNumberBase& y);
	friend BigNumberBase operator % (const BigNumberBase& x, const BigNumberBase& y);

	BigNumberCompare Compare(const BigNumberBase& x, const BigNumberBase& y);

	std::string toString();
	
private:
	static void TenExp(mpz_t& num, int32_t exp);

private:
	mpz_t m_integer;
	int32_t m_prec;
};