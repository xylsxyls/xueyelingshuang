#pragma once
#include <stdint.h>
#include "FastNumberMacro.h"
#include <string>

class FastNumberAPI FastNumber
{
public:
	enum BigNumberCompare
	{
		/** 第一个比第二个大
		*/
		BIG,

		/** 第一个比第二个小
		*/
		SMALL,

		/** 相等
		*/
		EQUAL

	};
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
	FastNumber();
	~FastNumber();
	FastNumber(int32_t num);
	FastNumber(const char* num);
	FastNumber(double num);
	FastNumber(const FastNumber& num);

public:
	FastNumber operator = (int32_t num);
	FastNumber operator = (const char* num);
	FastNumber operator = (double num);
	FastNumber operator = (const FastNumber& num);

public:
	friend inline FastNumber operator + (const FastNumber& x, const FastNumber& y)
	{
		return FastNumber::add(x, y);
	}
	friend inline FastNumber operator - (const FastNumber& x, const FastNumber& y)
	{
		return FastNumber::sub(x, y);
	}
	friend inline FastNumber operator * (const FastNumber& x, const FastNumber& y)
	{
		return FastNumber::mul(x, y);
	}
	//除法小数位数和尾数处理规则和x内的除法规则一致
	friend inline FastNumber operator / (const FastNumber& x, const FastNumber& y)
	{
		return FastNumber::div(x, y);
	}
	friend inline FastNumber operator % (const FastNumber& x, const FastNumber& y)
	{
		return FastNumber::mod(x, y);
	}

	friend inline bool operator == (const FastNumber& x, const FastNumber& y)
	{
		return FastNumber::equal(x, y);
	}
	friend inline bool operator != (const FastNumber& x, const FastNumber& y)
	{
		return FastNumber::notEqual(x, y);
	}
	friend inline bool operator >  (const FastNumber& x, const FastNumber& y)
	{
		return FastNumber::bigThan(x, y);
	}
	friend inline bool operator >= (const FastNumber& x, const FastNumber& y)
	{
		return FastNumber::bigEqual(x, y);
	}
	friend inline bool operator <  (const FastNumber& x, const FastNumber& y)
	{
		return FastNumber::smallThan(x, y);
	}
	friend inline bool operator <= (const FastNumber& x, const FastNumber& y)
	{
		return FastNumber::smallEqual(x, y);
	}

	FastNumber operator ++ ();//前++
	FastNumber operator -- ();//前--
	FastNumber operator ++ (int);//后++
	FastNumber operator -- (int);//后--

	FastNumber pow(const FastNumber& num);

	std::string toString() const;

	/** 返回数若为除数，且为0时不崩溃，除后返回0，含有精度
	*/
	FastNumber zero() const;

	FastNumber abs() const;

	FastNumber toPrec(int32_t prec, PrecFlag flag = HALF_ADJUST) const;

	//返回本身
	FastNumber& setPrec(int32_t prec, PrecFlag flag = HALF_ADJUST);

	static FastNumber minNumber(const FastNumber& x, const FastNumber& y);
	static FastNumber maxNumber(const FastNumber& x, const FastNumber& y);

private:
	static FastNumber add(const FastNumber& x, const FastNumber& y);
	static FastNumber sub(const FastNumber& x, const FastNumber& y);
	static FastNumber mul(const FastNumber& x, const FastNumber& y);
	static FastNumber div(const FastNumber& x, const FastNumber& y);
	static FastNumber mod(const FastNumber& x, const FastNumber& y);

	static bool equal(const FastNumber& x, const FastNumber& y);
	static bool notEqual(const FastNumber& x, const FastNumber& y);
	static bool bigThan(const FastNumber& x, const FastNumber& y);
	static bool bigEqual(const FastNumber& x, const FastNumber& y);
	static bool smallThan(const FastNumber& x, const FastNumber& y);
	static bool smallEqual(const FastNumber& x, const FastNumber& y);

	//只比较大小不比较精度
	static BigNumberCompare Compare(const FastNumber& x, const FastNumber& y);

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
	int64_t m_base;
	int32_t m_prec;
	bool m_isZero;
};