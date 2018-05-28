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
		/** ��һ���ȵڶ�����
		*/
		BIG,

		/** ��һ���ȵڶ���С
		*/
		SMALL,

		/** ���
		*/
		EQUAL
	};
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
	enum Calc
	{
		PRECISE = 16
	};
public:
	/** ���캯��
	*/
	BigNumberBase();

	/** ��������
	*/
	~BigNumberBase();

	/** ���캯��
	@param [in] num ��ֵ�ַ���
	*/
	BigNumberBase(const char* num);

	/** �������캯��
	@param [in] num ��ֵ��
	*/
	BigNumberBase(const BigNumberBase& num);

	/** ��ֵ����
	@param [in] num ��ֵ��
	@return ���ظ�ֵ�����
	*/
	BigNumberBase operator = (const BigNumberBase& num);

public:
	friend BigNumberBase operator + (const BigNumberBase& x, const BigNumberBase& y);
	friend BigNumberBase operator - (const BigNumberBase& x, const BigNumberBase& y);
	friend BigNumberBase operator * (const BigNumberBase& x, const BigNumberBase& y);
	friend BigNumberBase operator / (const BigNumberBase& x, const BigNumberBase& y);
	friend BigNumberBase operator % (const BigNumberBase& x, const BigNumberBase& y);

	BigNumberBase div(const BigNumberBase& divisor, int32_t prec = 16, PrecFlag flag = HALF_ADJUST);

	/** �˷���ָ������������int�ķ�Χ֮��
	*/
	BigNumberBase pow(const BigNumberBase& powNum, int32_t prec = 16, PrecFlag flag = HALF_ADJUST);

	//ֻ�Ƚϴ�С���ȽϾ���
	static BigNumberCompare Compare(const BigNumberBase& x, const BigNumberBase& y);

	std::string toString() const;

	void setFixedPrec(int32_t fixedPrec, PrecFlag fixedPrecFlag = HALF_ADJUST);

	void setPrec(int32_t prec, PrecFlag flag = HALF_ADJUST);
	
private:
	//num*10��exp�η�
	static void TenExp(mpz_t& num, int32_t exp);

public:
	mpz_t m_integer;
	int32_t m_prec;
	int32_t m_fixedPrec;
	PrecFlag m_fixedPrecFlag;
};