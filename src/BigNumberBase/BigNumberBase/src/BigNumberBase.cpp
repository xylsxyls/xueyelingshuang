#include "BigNumberBase.h"
#include "CStringManager/CStringManagerAPI.h"
#include <math.h>
#include "gmp.h"
#ifdef _WIN64
#pragma comment(lib, "libgmp.dll.lib")
#elif _WIN32
#pragma comment(lib, "gmp.lib")
#endif

class GmpInt
{
public:
	static void TenExp(mpz_t& num, int32_t exp);

	mpz_t m_integer;
};

BigNumberBase::BigNumberBase():
m_prec(0),
m_fixedPrec(-1),
m_fixedPrecFlag(HALF_ADJUST)
{
	m_gmp = new GmpInt;
	mpz_init(m_gmp->m_integer);
}

BigNumberBase::~BigNumberBase()
{
	mpz_clear(m_gmp->m_integer);
	delete m_gmp;
}

BigNumberBase::BigNumberBase(const char* num):
m_prec(0),
m_fixedPrec(-1),
m_fixedPrecFlag(HALF_ADJUST)
{
	m_gmp = new GmpInt;
	std::vector<std::string> vecNum = CStringManager::split(num, ".");
	if (vecNum.size() == 1)
	{
		mpz_init_set_str(m_gmp->m_integer, num, 10);
	}
	else if (vecNum.size() == 2)
	{
		m_prec = vecNum[1].size();
		std::string strNum = num;
		CStringManager::Replace(strNum, ".", "");
		mpz_init_set_str(m_gmp->m_integer, strNum.c_str(), 10);
	}
}

BigNumberBase::BigNumberBase(const BigNumberBase& num):
m_prec(num.m_prec),
m_fixedPrec(-1),
m_fixedPrecFlag(HALF_ADJUST)
{
	m_gmp = new GmpInt;
	mpz_init_set(m_gmp->m_integer, num.m_gmp->m_integer);
}

BigNumberBase BigNumberBase::operator = (const BigNumberBase& num)
{
	mpz_set(m_gmp->m_integer, num.m_gmp->m_integer);
	m_prec = num.m_prec;
	if (m_fixedPrec != -1)
	{
		setPrec(m_fixedPrec, m_fixedPrecFlag);
	}
	return *this;
}

//num*10的exp次方
void GmpInt::TenExp(mpz_t& num, int32_t exp)
{
	if (exp == 0)
	{
		return;
	}
	char* szExp = (char*)malloc(exp + 2);
	memset(szExp, 48, exp + 1);
	szExp[exp + 1] = 0;
	szExp[0] = 49;
	mpz_t mpzExp;
	mpz_init_set_str(mpzExp, szExp, 10);
	mpz_mul(num, num, mpzExp);
	mpz_clear(mpzExp);
	::free(szExp);
}

BigNumberBase BigNumberBase::add(const BigNumberBase& x, const BigNumberBase& y)
{
	BigNumberBase result;
	if (x.m_prec == y.m_prec)
	{
		mpz_add(result.m_gmp->m_integer, x.m_gmp->m_integer, y.m_gmp->m_integer);
		result.m_prec = x.m_prec;
	}
	else if (x.m_prec < y.m_prec)
	{
		result = x;
		GmpInt::TenExp(result.m_gmp->m_integer, y.m_prec - x.m_prec);
		result.m_prec = y.m_prec;
		mpz_add(result.m_gmp->m_integer, result.m_gmp->m_integer, y.m_gmp->m_integer);
	}
	else
	{
		result = y;
		GmpInt::TenExp(result.m_gmp->m_integer, x.m_prec - y.m_prec);
		result.m_prec = x.m_prec;
		mpz_add(result.m_gmp->m_integer, x.m_gmp->m_integer, result.m_gmp->m_integer);
	}
	return result;
}

BigNumberBase BigNumberBase::sub(const BigNumberBase& x, const BigNumberBase& y)
{
	BigNumberBase result;
	if (x.m_prec == y.m_prec)
	{
		result.m_prec = x.m_prec;
		mpz_sub(result.m_gmp->m_integer, x.m_gmp->m_integer, y.m_gmp->m_integer);
	}
	else if (x.m_prec < y.m_prec)
	{
		result = x;
		GmpInt::TenExp(result.m_gmp->m_integer, y.m_prec - x.m_prec);
		result.m_prec = y.m_prec;
		mpz_sub(result.m_gmp->m_integer, result.m_gmp->m_integer, y.m_gmp->m_integer);
	}
	else
	{
		result = y;
		GmpInt::TenExp(result.m_gmp->m_integer, x.m_prec - y.m_prec);
		result.m_prec = x.m_prec;
		mpz_sub(result.m_gmp->m_integer, x.m_gmp->m_integer, result.m_gmp->m_integer);
	}
	return result;
}

BigNumberBase BigNumberBase::mul(const BigNumberBase& x, const BigNumberBase& y)
{
	BigNumberBase result;
	mpz_mul(result.m_gmp->m_integer, x.m_gmp->m_integer, y.m_gmp->m_integer);
	result.m_prec = x.m_prec + y.m_prec;
	return result;
}

BigNumberBase BigNumberBase::div(const BigNumberBase& x, const BigNumberBase& y)
{
	if (x.m_prec != 0 || y.m_prec != 0)
	{
		abort();
	}
	BigNumberBase result;
	mpz_tdiv_q(result.m_gmp->m_integer, x.m_gmp->m_integer, y.m_gmp->m_integer);
	return result;
}

BigNumberBase BigNumberBase::mod(const BigNumberBase& x, const BigNumberBase& y)
{
	return sub(x, mul(div(x, y), y));
}

BigNumberBase BigNumberBase::div(const BigNumberBase& divisor, int32_t prec, PrecFlag flag)
{
	if (prec == -1)
	{
		prec = Calc::PRECISE;
	}
	if (m_prec >= divisor.m_prec)
	{
		BigNumberBase dividerTemp = *this;
		GmpInt::TenExp(dividerTemp.m_gmp->m_integer, prec + Calc::PRECISE);
		BigNumberBase divisorTemp = divisor;
		GmpInt::TenExp(divisorTemp.m_gmp->m_integer, m_prec - divisor.m_prec);
		dividerTemp.m_prec = 0;
		divisorTemp.m_prec = 0;

		BigNumberBase result;
		mpz_tdiv_q(result.m_gmp->m_integer, dividerTemp.m_gmp->m_integer, divisorTemp.m_gmp->m_integer);
		result.m_prec = prec + Calc::PRECISE;
		result.setPrec(prec, flag);
		return result;
	}
	else
	{
		BigNumberBase dividerTemp = *this;
		GmpInt::TenExp(dividerTemp.m_gmp->m_integer, divisor.m_prec - m_prec + prec + Calc::PRECISE);
		BigNumberBase divisorTemp = divisor;
		dividerTemp.m_prec = 0;
		divisorTemp.m_prec = 0;

		BigNumberBase result;
		mpz_tdiv_q(result.m_gmp->m_integer, dividerTemp.m_gmp->m_integer, divisorTemp.m_gmp->m_integer);
		result.m_prec = prec + Calc::PRECISE;
		result.setPrec(prec, flag);
		return result;
	}
}

BigNumberBase BigNumberBase::pow(const BigNumberBase& powNum, int32_t prec, PrecFlag flag)
{
	if (prec == -1)
	{
		prec = Calc::PRECISE;
	}
	BigNumberBase result;
	if (BigNumberBase::Compare(powNum, "0") == EQUAL)
	{
		result = "1";
		result.setPrec(prec, flag);
		return result;
	}
	
	BigNumberBase powNumTemp = powNum;
	bool powIsMinus = (BigNumberBase::Compare(powNumTemp, "0") == SMALL);
	if (powIsMinus)
	{
		powNumTemp = mul(powNumTemp, "-1");
		std::string sds = powNumTemp.toString();
		sds = sds;
	}
	powNumTemp.m_prec = 0;
	unsigned long int index = strtoul(powNumTemp.toString().c_str(), nullptr, 10);
	unsigned long int indexNum = (unsigned long int)::pow(10, powNum.m_prec);
	
	bool isMinus = (BigNumberBase::Compare(*this, "0") == SMALL);
	if (isMinus)
	{
		//先判断是否有效
		unsigned long int indexBk = index;
		unsigned long int indexNumBk = indexNum;
		while (indexBk % 2 == 0 && indexNumBk % 2 == 0)
		{
			indexBk = indexBk / 2;
			indexNumBk = indexNumBk / 2;
		}
		if (indexBk % 2 == 0)
		{
			isMinus = false;
		}
		else
		{
			BigNumberBase resultBk = result;
			mpz_pow_ui(resultBk.m_gmp->m_integer, m_gmp->m_integer, indexBk);
			mpz_root(resultBk.m_gmp->m_integer, resultBk.m_gmp->m_integer, indexNumBk);
			isMinus = true;
		}
		*this = mul(*this, "-1");
	}
	
	mpz_pow_ui(result.m_gmp->m_integer, m_gmp->m_integer, index);
	GmpInt::TenExp(result.m_gmp->m_integer, (unsigned long int)::pow(10, powNum.m_prec) * (prec + Calc::PRECISE));
	unsigned long int exp = (unsigned long int)::pow(10, powNum.m_prec);
	mpz_root(result.m_gmp->m_integer, result.m_gmp->m_integer, exp);
	result.m_prec = prec + Calc::PRECISE;

	BigNumberBase divisorMultiple = "1";
	GmpInt::TenExp(divisorMultiple.m_gmp->m_integer, m_prec);
	mpz_pow_ui(divisorMultiple.m_gmp->m_integer, divisorMultiple.m_gmp->m_integer, index);
	GmpInt::TenExp(divisorMultiple.m_gmp->m_integer, exp * (prec + Calc::PRECISE));
	mpz_root(divisorMultiple.m_gmp->m_integer, divisorMultiple.m_gmp->m_integer, exp);
	divisorMultiple.m_prec = prec + Calc::PRECISE;

	result = result.div(divisorMultiple, prec + Calc::PRECISE, flag);

	if (powIsMinus)
	{
		BigNumberBase minusRoot = "1";
		result = minusRoot.div(result, prec + Calc::PRECISE, flag);
	}

	result.setPrec(prec);
	if (isMinus)
	{
		result = mul(result, "-1");
	}
	
	return result;
}

BigNumberBase::BigNumberCompare BigNumberBase::Compare(const BigNumberBase& x, const BigNumberBase& y)
{
	BigNumberBase result = sub(x, y);
	std::string strResult = result.toString();
	char firstChara = strResult[0];
	char lastChara = strResult[strResult.size() - 1];
	if (firstChara == '-')
	{
		return BigNumberCompare::SMALL;
	}
	else
	{
		int32_t count = mpz_sizeinbase(result.m_gmp->m_integer, 10);
		if (count == 1 && firstChara == '0' && lastChara == '0')
		{
			return BigNumberCompare::EQUAL;
		}
		return BigNumberCompare::BIG;
	}
}

std::string BigNumberBase::toString() const
{
	int32_t count = mpz_sizeinbase(m_gmp->m_integer, 10);
	char* num = (char*)::malloc(count + 2);
	mpz_get_str(num, 10, m_gmp->m_integer);
	std::string result = num;
	if (m_prec == 0)
	{
		::free(num);
		return result;
	}
	bool isMinus = (result[0] == '-');
	int32_t countValid = result.size() - (isMinus ? 1 : 0);
	int32_t insertCount = m_prec - countValid + 1;
	if (insertCount > 0)
	{
		result.insert(isMinus ? 1 : 0, insertCount, '0');
	}
	result.insert(result.size() - m_prec, 1, '.');
	::free(num);
	return result;
}

void BigNumberBase::setFixedPrec(int32_t fixedPrec, PrecFlag fixedPrecFlag)
{
	if (fixedPrec == -1)
	{
		fixedPrec = Calc::PRECISE;
	}
	m_fixedPrec = fixedPrec;
	m_fixedPrecFlag = fixedPrecFlag;
	setPrec(m_fixedPrec, m_fixedPrecFlag);
}

void BigNumberBase::setPrec(int32_t prec, PrecFlag flag)
{
	if (prec == -1)
	{
		prec = Calc::PRECISE;
	}
	if (m_prec == prec)
	{
		return;
	}
	if (m_prec < prec)
	{
		GmpInt::TenExp(m_gmp->m_integer, (prec - m_prec));
		m_prec = prec;
		return;
	}
	if (flag == BigNumberBase::HALF_ADJUST)
	{
		std::string result = toString();
		if (result[result.size() - (m_prec - prec)] < '5')
		{
			setPrec(prec, BigNumberBase::ROUND_OFF);
			return;
		}

		if (BigNumberBase::Compare(*this, "0") == BigNumberCompare::BIG)
		{
			setPrec(prec, BigNumberBase::ROUND_UP);
			return;
		}
		setPrec(prec, BigNumberBase::ROUND_DOWN);
		return;
	}

	int32_t exp = m_prec - prec;
	char* szExp = (char*)malloc(exp + 2);
	memset(szExp, 48, exp + 1);
	szExp[exp + 1] = 0;
	szExp[0] = 49;
	mpz_t mpzExp;
	mpz_init_set_str(mpzExp, szExp, 10);
	
	switch (flag)
	{
	case BigNumberBase::ROUND_OFF:
	{
		mpz_tdiv_q(m_gmp->m_integer, m_gmp->m_integer, mpzExp);
		break;
	}
	case BigNumberBase::ROUND_UP:
	{
		mpz_cdiv_q(m_gmp->m_integer, m_gmp->m_integer, mpzExp);
		break;
	}
	case BigNumberBase::ROUND_DOWN:
	{
		mpz_fdiv_q(m_gmp->m_integer, m_gmp->m_integer, mpzExp);
		break;
	}
	default:
		break;
	}

	mpz_clear(mpzExp);
	::free(szExp);
	m_prec = prec;
}

//#include <string>
//#include <iostream>
//#include "D:\\SendToMessageTest.h"
//
//int main()
//{
//	BigNumberBase x = "-0.2";
//	BigNumberBase y = "0.4";
//	RCSend("x = %s,y = %s\n", x.toString().c_str(), y.toString().c_str());
//	RCSend("x + y = %s\n", (x + y).toString().c_str());
//	RCSend("x - y = %s\n", (x - y).toString().c_str());
//	RCSend("x * y = %s\n", (x * y).toString().c_str());
//	RCSend("x / y = %s\n", x.div(y).toString().c_str());
//	RCSend("x ^ y = %s\n", x.pow(y).toString().c_str());
//	RCSend("\n");
//	std::vector<std::string> xVec;
//	xVec.push_back("0.2");
//	xVec.push_back("-0.2");
//	xVec.push_back("2.2");
//	xVec.push_back("-2.2");
//	std::vector<std::string> yVec;
//	yVec.push_back("0.2");
//	yVec.push_back("-0.2");
//	yVec.push_back("2.2");
//	yVec.push_back("-2.2");
//	for (unsigned int xIndex = 0; xIndex < xVec.size(); xIndex++)
//	{
//		for (unsigned int yIndex = 0; yIndex < yVec.size(); yIndex++)
//		{
//			x = xVec[xIndex].c_str();
//			y = yVec[yIndex].c_str();
//			RCSend("x = %s,y = %s\n", x.toString().c_str(), y.toString().c_str());
//			//RCSend("x + y = %s\n", (x + y).toString().c_str());
//			//RCSend("x - y = %s\n", (x - y).toString().c_str());
//			//RCSend("x * y = %s\n", (x * y).toString().c_str());
//			//RCSend("x / y = %s\n", x.div(y).toString().c_str());
//			if (BigNumberBase::Compare(x, "0") == BigNumberBase::SMALL)
//			{
//				RCSend("\n");
//				continue;
//			}
//			RCSend("x ^ y = %s\n", x.pow(y).toString().c_str());
//			RCSend("\n");
//		}
//	}
//	getchar();
//	return 0;
//}