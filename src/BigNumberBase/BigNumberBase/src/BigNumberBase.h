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
	/** ���캯��
	*/
	BigNumberBase();

	/** ���캯��
	@param [in] num ��ֵ
	*/
	BigNumberBase(int32_t num);

	/** ���캯��
	@param [in] num ��ֵ�ַ���
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