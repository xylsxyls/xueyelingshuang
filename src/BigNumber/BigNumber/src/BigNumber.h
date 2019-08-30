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
		/** ȥβ
		*/
		ROUND_OFF,

		/** ��������
		*/
		HALF_ADJUST,

		/** ��������һλ
		*/
		ROUND_UP,

		/** ��������һλ
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
	//����С��λ����β����������x�ڵĳ�������һ��
	friend BigNumber operator / (const BigNumber& x, const BigNumber& y)
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
	friend bool operator <  (const BigNumber& x, const BigNumber& y)
	{
		return BigNumber::smallThan(x, y);
	}
	friend inline bool operator <= (const BigNumber& x, const BigNumber& y)
	{
		return BigNumber::smallEqual(x, y);
	}

	inline BigNumber operator ++ ();//ǰ++
	inline BigNumber operator -- ();//ǰ--
	inline BigNumber operator ++ (int);//��++
	inline BigNumber operator -- (int);//��--

	inline BigNumber pow(const BigNumber& powNum, int32_t prec = 16, PrecFlag flag = HALF_ADJUST);

	std::string toString() const;

	/** ��������Ϊ��������Ϊ0ʱ�����������󷵻�0�����о���
	*/
	inline BigNumber zero() const;

	inline BigNumber abs() const;

	inline BigNumber toPrec(int32_t prec, PrecFlag flag = HALF_ADJUST) const;

	//����3�������Ƿ��ر���
	BigNumber& setPrec(int32_t prec, PrecFlag flag = HALF_ADJUST);

	BigNumber& setFixedPrec(int32_t fixedPrec, PrecFlag fixedPrecFlag = HALF_ADJUST);

	//��������
	BigNumber& setDivParam(int32_t prec = 16, PrecFlag flag = HALF_ADJUST);

	static inline BigNumber minNumber(const BigNumber& x, const BigNumber& y);
	static inline BigNumber maxNumber(const BigNumber& x, const BigNumber& y);

private:
	static inline BigNumber add(const BigNumber& x, const BigNumber& y);
	static inline BigNumber sub(const BigNumber& x, const BigNumber& y);
	static inline BigNumber mul(const BigNumber& x, const BigNumber& y);
	static inline BigNumber div(const BigNumber& x, const BigNumber& y);
	static inline BigNumber mod(const BigNumber& x, const BigNumber& y);

	static inline bool equal(const BigNumber& x, const BigNumber& y);
	static inline bool notEqual(const BigNumber& x, const BigNumber& y);
	static inline bool bigThan(const BigNumber& x, const BigNumber& y);
	static inline bool bigEqual(const BigNumber& x, const BigNumber& y);
	static inline bool smallThan(const BigNumber& x, const BigNumber& y);
	static inline bool smallEqual(const BigNumber& x, const BigNumber& y);

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