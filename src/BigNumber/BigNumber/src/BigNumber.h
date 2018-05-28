#pragma once
#include <stdint.h>
#include "BigNumberMacro.h"
#include <string>

class BigNumberBase;
class BigNumberAPI BigNumber
{
public:
	enum PrecFlag
	{
		/** 去尾
		*/
		ROUND_OFF,

		/** 四舍五入
		*/
		HALF_ADJUST,

		/** 总是向上一位
		*/
		ROUND_UP,

		/** 总是向下一位
		*/
		ROUND_DOWN
	};
public:
	BigNumber();
	BigNumber(int32_t num);
	BigNumber(const char* num);
	BigNumber(double num);
	BigNumber(const BigNumber& num);

public:
	BigNumber operator = (int32_t num);
	BigNumber operator = (const char* num);
	BigNumber operator = (double num);

public:
	friend BigNumber operator + (const BigNumber& x, const BigNumber& y);
	friend BigNumber operator - (const BigNumber& x, const BigNumber& y);
	friend BigNumber operator * (const BigNumber& x, const BigNumber& y);
	friend BigNumber operator / (const BigNumber& x, const BigNumber& y);
	friend BigNumber operator % (const BigNumber& x, const BigNumber& y);

	friend bool operator == (const BigNumber& x, const BigNumber& y);
	friend bool operator != (const BigNumber& x, const BigNumber& y);
	friend bool operator >  (const BigNumber& x, const BigNumber& y);
	friend bool operator >= (const BigNumber& x, const BigNumber& y);
	friend bool operator <  (const BigNumber& x, const BigNumber& y);
	friend bool operator <= (const BigNumber& x, const BigNumber& y);

	BigNumber operator ++ ();//前++
	BigNumber operator -- ();//前--
	BigNumber operator ++ (int);//后++
	BigNumber operator -- (int);//后--

	BigNumber pow(const BigNumber& powNum, int32_t prec = 16, PrecFlag flag = HALF_ADJUST);

	std::string toString();

	void setPrec(int32_t prec, PrecFlag flag = HALF_ADJUST);

	void setFixedPrec(int32_t fixedPrec, PrecFlag fixedPrecFlag = HALF_ADJUST);

	void setDivParam(int32_t prec, PrecFlag flag = HALF_ADJUST);

private:
	static BigNumber add(const BigNumber& x, const BigNumber& y);
	static BigNumber sub(const BigNumber& x, const BigNumber& y);
	static BigNumber mul(const BigNumber& x, const BigNumber& y);
	static BigNumber div(const BigNumber& x, const BigNumber& y);
	static BigNumber mod(const BigNumber& x, const BigNumber& y);

	static bool equal(const BigNumber& x, const BigNumber& y);
	static bool notEqual(const BigNumber& x, const BigNumber& y);
	static bool big(const BigNumber& x, const BigNumber& y);
	static bool bigEqual(const BigNumber& x, const BigNumber& y);
	static bool small(const BigNumber& x, const BigNumber& y);
	static bool smallEqual(const BigNumber& x, const BigNumber& y);

private:
	BigNumberBase* m_base;
	int32_t m_prec;
	PrecFlag m_flag;
};

#include "BigNumber.inl"