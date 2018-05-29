#pragma once
#include "BigNumberBaseMacro.h"
#include <stdint.h>
#include <string>

class GmpInt;
class BigNumberBaseAPI BigNumberBase
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
	enum Calc
	{
		PRECISE = 16
	};
public:
	/** 构造函数
	*/
	BigNumberBase();

	/** 析构函数
	*/
	~BigNumberBase();

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
	//friend BigNumberBase operator + (const BigNumberBase& x, const BigNumberBase& y);
	//friend BigNumberBase operator - (const BigNumberBase& x, const BigNumberBase& y);
	//friend BigNumberBase operator * (const BigNumberBase& x, const BigNumberBase& y);
	//friend BigNumberBase operator / (const BigNumberBase& x, const BigNumberBase& y);
	//friend BigNumberBase operator % (const BigNumberBase& x, const BigNumberBase& y);

	BigNumberBase div(const BigNumberBase& divisor, int32_t prec = 16, PrecFlag flag = HALF_ADJUST);

	/** 乘方，指数必须在两倍int的范围之内
	*/
	BigNumberBase pow(const BigNumberBase& powNum, int32_t prec = 16, PrecFlag flag = HALF_ADJUST);

	//只比较大小不比较精度
	static BigNumberCompare Compare(const BigNumberBase& x, const BigNumberBase& y);

	std::string toString() const;

	void setFixedPrec(int32_t fixedPrec, PrecFlag fixedPrecFlag = HALF_ADJUST);

	void setPrec(int32_t prec, PrecFlag flag = HALF_ADJUST);

public:
	static BigNumberBase add(const BigNumberBase& x, const BigNumberBase& y);
	static BigNumberBase sub(const BigNumberBase& x, const BigNumberBase& y);
	static BigNumberBase mul(const BigNumberBase& x, const BigNumberBase& y);
	static BigNumberBase div(const BigNumberBase& x, const BigNumberBase& y);
	static BigNumberBase mod(const BigNumberBase& x, const BigNumberBase& y);

public:
	GmpInt* m_gmp;
	int32_t m_prec;
	int32_t m_fixedPrec;
	PrecFlag m_fixedPrecFlag;
};

#include "BigNumberBase.inl"