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
	@param [in] num 赋值类
	*/
	BigNumberBase(const BigNumberBase& num);

	/** 赋值操作
	@param [in] num 赋值类
	@return 返回赋值后的类
	*/
	BigNumberBase operator = (const BigNumberBase& num);

public:
	friend BigNumberBase operator + (const BigNumberBase& x, const BigNumberBase& y);
	friend BigNumberBase operator - (const BigNumberBase& x, const BigNumberBase& y);
	friend BigNumberBase operator * (const BigNumberBase& x, const BigNumberBase& y);
	friend BigNumberBase operator / (const BigNumberBase& x, const BigNumberBase& y);
	friend BigNumberBase operator % (const BigNumberBase& x, const BigNumberBase& y);

	//只比较大小不比较精度
	static BigNumberCompare Compare(const BigNumberBase& x, const BigNumberBase& y);

	std::string toString();

	void setMaxPrec(int32_t maxPrec);
	
private:
	//num*10的exp次方
	static void TenExp(mpz_t& num, int32_t exp);

private:
	mpz_t m_integer;
	int32_t m_prec;
	int32_t m_maxPrec;
};