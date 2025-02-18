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
    ~BigNumber();
	BigNumber(int32_t num);
	BigNumber(const char* num);
	BigNumber(double num);
	BigNumber(const BigNumber& num);

public:
	BigNumber operator = (int32_t num);
	BigNumber operator = (const char* num);
	BigNumber operator = (double num);
    BigNumber operator = (const BigNumber& num);

public:
	friend inline BigNumber operator + (const BigNumber& x, const BigNumber& y)
	{
		return BigNumber::add(x, y);
	}
	friend inline BigNumber operator - (const BigNumber& x, const BigNumber& y)
	{
		return BigNumber::sub(x, y);
	}
	friend inline BigNumber operator * (const BigNumber& x, const BigNumber& y)
	{
		return BigNumber::mul(x, y);
	}
	//除法小数位数和尾数处理规则和x内的除法规则一致
	friend inline BigNumber operator / (const BigNumber& x, const BigNumber& y)
	{
		return BigNumber::div(x, y);
	}
	friend inline BigNumber operator % (const BigNumber& x, const BigNumber& y)
	{
		return BigNumber::mod(x, y);
	}

	friend inline bool operator == (const BigNumber& x, const BigNumber& y)
	{
		return BigNumber::equal(x, y);
	}
	friend inline bool operator != (const BigNumber& x, const BigNumber& y)
	{
		return BigNumber::notEqual(x, y);
	}
	friend inline bool operator >  (const BigNumber& x, const BigNumber& y)
	{
		return BigNumber::bigThan(x, y);
	}
	friend inline bool operator >= (const BigNumber& x, const BigNumber& y)
	{
		return BigNumber::bigEqual(x, y);
	}
	friend inline bool operator <  (const BigNumber& x, const BigNumber& y)
	{
		return BigNumber::smallThan(x, y);
	}
	friend inline bool operator <= (const BigNumber& x, const BigNumber& y)
	{
		return BigNumber::smallEqual(x, y);
	}

	BigNumber operator ++ ();//前++
	BigNumber operator -- ();//前--
	BigNumber operator ++ (int);//后++
	BigNumber operator -- (int);//后--

	BigNumber pow(const BigNumber& powNum, int32_t prec = 16, PrecFlag flag = HALF_ADJUST);

	std::string toString() const;

	/** 返回数若为除数，且为0时不崩溃，除后返回0，含有精度
	*/
	BigNumber zero() const;

	BigNumber abs() const;

	BigNumber toPrec(int32_t prec, PrecFlag flag = HALF_ADJUST) const;

	//以下3函数都是返回本身
	BigNumber& setPrec(int32_t prec, PrecFlag flag = HALF_ADJUST);

	BigNumber& setFixedPrec(int32_t fixedPrec, PrecFlag fixedPrecFlag = HALF_ADJUST);

	//除法规则
	BigNumber& setDivParam(int32_t prec = 16, PrecFlag flag = HALF_ADJUST);

	static BigNumber minNumber(const BigNumber& x, const BigNumber& y);
	static BigNumber maxNumber(const BigNumber& x, const BigNumber& y);

private:
	static BigNumber add(const BigNumber& x, const BigNumber& y);
	static BigNumber sub(const BigNumber& x, const BigNumber& y);
	static BigNumber mul(const BigNumber& x, const BigNumber& y);
	static BigNumber div(const BigNumber& x, const BigNumber& y);
	static BigNumber mod(const BigNumber& x, const BigNumber& y);

	static bool equal(const BigNumber& x, const BigNumber& y);
	static bool notEqual(const BigNumber& x, const BigNumber& y);
	static bool bigThan(const BigNumber& x, const BigNumber& y);
	static bool bigEqual(const BigNumber& x, const BigNumber& y);
	static bool smallThan(const BigNumber& x, const BigNumber& y);
	static bool smallEqual(const BigNumber& x, const BigNumber& y);

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
#ifdef _DEBUG
	std::string m_num;
#endif
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	PrecFlag m_divFlag;
	int32_t m_divPrec;
	BigNumberBase* m_base;
	bool m_isZero;
};