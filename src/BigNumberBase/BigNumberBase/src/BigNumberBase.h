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
	/** ���캯��
	*/
	BigNumberBase();

	/** ��������
	*/
	~BigNumberBase();

	/** ���캯��
	@param [in] num ��ֵ
	*/
	BigNumberBase(int32_t num);

	/** ���캯��
	@param [in] num ��ֵ�ַ���
	*/
	BigNumberBase(const char* num);

	/** �������캯��
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