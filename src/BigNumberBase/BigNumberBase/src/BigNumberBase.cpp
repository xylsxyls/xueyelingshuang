#include "BigNumberBase.h"
#include "CStringManager/CStringManagerAPI.h"
#include <math.h>
#include "gmp.h"
#pragma comment(lib, "libgmp.a")
#pragma comment(lib, "libgcc.a")
#pragma comment(lib, "libmingwex.a")
#pragma comment(lib, "libmsvcrt.a")

BigNumberBase::BigNumberBase():
m_prec(0),
m_fixedPrec(-1),
m_fixedPrecFlag(HALF_ADJUST)
{
	mpz_init(m_integer);
}

BigNumberBase::~BigNumberBase()
{
	mpz_clear(m_integer);
}

BigNumberBase::BigNumberBase(const char* num):
m_prec(0),
m_fixedPrec(-1),
m_fixedPrecFlag(HALF_ADJUST)
{
	std::vector<std::string> vecNum = CStringManager::split(num, ".");
	if (vecNum.size() == 1)
	{
		mpz_init_set_str(m_integer, num, 10);
	}
	else if (vecNum.size() == 2)
	{
		m_prec = vecNum[1].size();
		std::string strNum = num;
		CStringManager::Replace(strNum, ".", "");
		mpz_init_set_str(m_integer, strNum.c_str(), 10);
	}
}

BigNumberBase::BigNumberBase(const BigNumberBase& num):
m_prec(num.m_prec),
m_fixedPrec(-1),
m_fixedPrecFlag(HALF_ADJUST)
{
	mpz_init_set(m_integer, num.m_integer);
}

BigNumberBase BigNumberBase::operator = (const BigNumberBase& num)
{
	mpz_set(m_integer, num.m_integer);
	m_prec = num.m_prec;
	if (m_fixedPrec != -1)
	{
		setPrec(m_fixedPrec, m_fixedPrecFlag);
	}
	return *this;
}

void BigNumberBase::TenExp(mpz_t& num, int32_t exp)
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

BigNumberBase operator + (const BigNumberBase& x, const BigNumberBase& y)
{
	BigNumberBase result;
	if (x.m_prec == y.m_prec)
	{
		mpz_add(result.m_integer, x.m_integer, y.m_integer);
	}
	else if (x.m_prec < y.m_prec)
	{
		result = x;
		BigNumberBase::TenExp(result.m_integer, y.m_prec - x.m_prec);
		result.m_prec = y.m_prec;
		mpz_add(result.m_integer, result.m_integer, y.m_integer);
	}
	else
	{
		result = y;
		BigNumberBase::TenExp(result.m_integer, x.m_prec - y.m_prec);
		result.m_prec = x.m_prec;
		mpz_add(result.m_integer, x.m_integer, result.m_integer);
	}
	return result;
}

BigNumberBase operator - (const BigNumberBase& x, const BigNumberBase& y)
{
	BigNumberBase result;
	if (x.m_prec == y.m_prec)
	{
		result.m_prec = x.m_prec;
		mpz_sub(result.m_integer, x.m_integer, y.m_integer);
	}
	else if (x.m_prec < y.m_prec)
	{
		result = x;
		BigNumberBase::TenExp(result.m_integer, y.m_prec - x.m_prec);
		result.m_prec = y.m_prec;
		mpz_sub(result.m_integer, result.m_integer, y.m_integer);
	}
	else
	{
		result = y;
		BigNumberBase::TenExp(result.m_integer, x.m_prec - y.m_prec);
		result.m_prec = x.m_prec;
		mpz_sub(result.m_integer, x.m_integer, result.m_integer);
	}
	return result;
}

BigNumberBase operator * (const BigNumberBase& x, const BigNumberBase& y)
{
	BigNumberBase result;
	mpz_mul(result.m_integer, x.m_integer, y.m_integer);
	result.m_prec = x.m_prec + y.m_prec;
	return result;
}

BigNumberBase operator / (const BigNumberBase& x, const BigNumberBase& y)
{
	if (x.m_prec != 0 || y.m_prec != 0)
	{
		abort();
	}
	BigNumberBase result;
	mpz_tdiv_q(result.m_integer, x.m_integer, y.m_integer);
	return result;
}

BigNumberBase operator % (const BigNumberBase& x, const BigNumberBase& y)
{
	return x - x / y * y;
}

BigNumberBase BigNumberBase::div(const BigNumberBase& divisor, int32_t prec, PrecFlag flag)
{
	if (m_prec >= divisor.m_prec)
	{
		BigNumberBase dividerTemp = *this;
		BigNumberBase::TenExp(dividerTemp.m_integer, prec + 1);
		BigNumberBase divisorTemp = divisor;
		BigNumberBase::TenExp(divisorTemp.m_integer, m_prec - divisor.m_prec);
		dividerTemp.m_prec = 0;
		divisorTemp.m_prec = 0;

		BigNumberBase result;
		mpz_tdiv_q(result.m_integer, dividerTemp.m_integer, divisorTemp.m_integer);
		result.m_prec = prec + 1;
		result.setPrec(prec, flag);
		return result;
	}
	else
	{
		BigNumberBase dividerTemp = *this;
		BigNumberBase::TenExp(dividerTemp.m_integer, divisor.m_prec - m_prec + prec + 1);
		BigNumberBase divisorTemp = divisor;
		dividerTemp.m_prec = 0;
		divisorTemp.m_prec = 0;

		BigNumberBase result;
		mpz_tdiv_q(result.m_integer, dividerTemp.m_integer, divisorTemp.m_integer);
		result.m_prec = prec + 1;
		result.setPrec(prec, flag);
		return result;
	}
}

BigNumberBase BigNumberBase::pow(const BigNumberBase& powNum, int32_t prec, PrecFlag flag)
{
	//void mpz_pow_ui(mpz_t rop, mpz_t base, unsigned long int exp);
	BigNumberBase result;
	if (BigNumberBase::Compare(powNum, "0") == EQUAL)
	{
		result = "1";
		result.setPrec(prec, flag);
		return result;
	}
	
	BigNumberBase powNumTemp = powNum;
	bool isMinus = (BigNumberBase::Compare(*this, "0") == SMALL);
	if (isMinus)
	{
		*this = *this * "-1";
	}
	bool powIsMinus = (BigNumberBase::Compare(powNumTemp, "0") == SMALL);
	if (powIsMinus)
	{
		powNumTemp = powNumTemp * "-1";
		std::string sds = powNumTemp.toString();
		sds = sds;
	}
	powNumTemp.m_prec = 0;
	unsigned long int sds = strtoul(powNumTemp.toString().c_str(), nullptr, 10);
	mpz_pow_ui(result.m_integer, m_integer, sds);
	std::string sdsdsd = result.toString();
	BigNumberBase::TenExp(result.m_integer, (unsigned long int)::pow(10, powNum.m_prec) * (prec + 20));
	sdsdsd = result.toString();
	unsigned long int exp = (unsigned long int)::pow(10, powNum.m_prec);
	mpz_root(result.m_integer, result.m_integer, exp);
	sdsdsd = result.toString();
	result.m_prec = prec + 20;
	sdsdsd = result.toString();
	

	BigNumberBase divisorMultiple = "1";
	BigNumberBase::TenExp(divisorMultiple.m_integer, m_prec);
	sdsdsd = divisorMultiple.toString();
	mpz_pow_ui(divisorMultiple.m_integer, divisorMultiple.m_integer, sds);
	BigNumberBase::TenExp(divisorMultiple.m_integer, exp * (prec + 20));
	sdsdsd = divisorMultiple.toString();
	mpz_root(divisorMultiple.m_integer, divisorMultiple.m_integer, exp);
	sdsdsd = divisorMultiple.toString();
	divisorMultiple.m_prec = prec + 20;
	//divisorMultiple.setPrec(prec + 20);
	sdsdsd = divisorMultiple.toString();
	//divisorMultiple.setPrec(prec + 20);

	result = result.div(divisorMultiple, prec + 20, flag);

	if (powIsMinus)
	{
		BigNumberBase minusRoot = "1";
		result = minusRoot.div(result, prec + 20, flag);
	}

	result.setPrec(prec);
	if (isMinus)
	{
		result = result * ((sds % 2 == 0) ? "1" : "-1");
	}
	
	return result;
}

BigNumberBase::BigNumberCompare BigNumberBase::Compare(const BigNumberBase& x, const BigNumberBase& y)
{
	BigNumberBase result = x - y;
	std::string strResult = result.toString();
	char firstChara = strResult[0];
	char lastChara = strResult[strResult.size() - 1];
	if (firstChara == '-')
	{
		return BigNumberCompare::SMALL;
	}
	else
	{
		int32_t count = mpz_sizeinbase(result.m_integer, 10);
		if (count == 1 && firstChara == '0' && lastChara == '0')
		{
			return BigNumberCompare::EQUAL;
		}
		return BigNumberCompare::BIG;
	}
}

std::string BigNumberBase::toString() const
{
	int32_t count = mpz_sizeinbase(m_integer, 10);
	char* num = (char*)::malloc(count + 2);
	mpz_get_str(num, 10, m_integer);
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
	m_fixedPrec = fixedPrec;
	m_fixedPrecFlag = fixedPrecFlag;
	setPrec(m_fixedPrec, m_fixedPrecFlag);
}

void BigNumberBase::setPrec(int32_t prec, PrecFlag flag)
{
	if (m_prec == prec)
	{
		return;
	}
	if (m_prec < prec)
	{
		BigNumberBase::TenExp(m_integer, (prec - m_prec));
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
		mpz_tdiv_q(m_integer, m_integer, mpzExp);
		break;
	}
	case BigNumberBase::ROUND_UP:
	{
		mpz_cdiv_q(m_integer, m_integer, mpzExp);
		break;
	}
	case BigNumberBase::ROUND_DOWN:
	{
		mpz_fdiv_q(m_integer, m_integer, mpzExp);
		break;
	}
	default:
		break;
	}

	mpz_clear(mpzExp);
	::free(szExp);
	m_prec = prec;
}

#include <string>
#include <iostream>

int main()
{
	int i = 1;
	while (i-- != 0)
	{
		BigNumberBase aaaaa = "5.12";
		BigNumberBase bbbbb = "7.1";
		BigNumberBase ccccc = aaaaa.div(bbbbb);
		//std::string str3 = ccccc.toString();
		std::string sdsd = aaaaa.toString();
		std::string sdsd2 = bbbbb.toString();
		std::string str2 = ccccc.toString();
	}
	//void mpz_mod (mpz_t r, mpz_t n, mpz_t d);
	//unsigned long int mpz_cdiv_qr_ui(mpz_t q, mpz_t r, mpz_t n, unsigned long int d);
	//void mpz_cdiv_q(mpz_t q, mpz_t n, mpz_t d);
	auto sdsdssdd = BigNumberBase::Compare("0.01", "0");
	BigNumberBase aaaaa = "-2";
	BigNumberBase bbbbb = "-3";
	BigNumberBase ccccc;
	ccccc.setFixedPrec(12);
	ccccc = aaaaa.pow(bbbbb, 10);
	//ccccc.setPrec(4, BigNumberBase::HALF_ADJUST);
	std::string str3 = ccccc.toString();
	std::string sdsd = aaaaa.toString();
	std::string sdsd2 = bbbbb.toString();
	std::string str2 = (aaaaa - bbbbb).toString();
	auto ss2 = BigNumberBase::Compare(aaaaa, bbbbb);
	std::string str = (aaaaa % bbbbb).toString();
	int x = 101;
	int y = 3;
	int z = x % y;

	aaaaa = "102";
	bbbbb = "3";
	str = (aaaaa % bbbbb).toString();
	x = 102;
	y = 3;
	z = x % y;

	aaaaa = "103";
	bbbbb = "3";
	str = (aaaaa % bbbbb).toString();
	x = 103;
	y = 3;
	z = x % y;

	aaaaa = "104";
	bbbbb = "3";
	str = (aaaaa % bbbbb).toString();
	x = 104;
	y = 3;
	z = x % y;

	aaaaa = "105";
	bbbbb = "3";
	str = (aaaaa % bbbbb).toString();
	x = 105;
	y = 3;
	z = x % y;

	mpz_t a, b, c, d;
	mpz_init(a);
	mpz_init(b);
	mpz_init(c);
	mpz_init(d);
	//计算2的1000次方
	mpz_init_set_ui(a, 2);
	mpz_pow_ui(c, a, 10);
	gmp_printf("c = %Zd\n", c);

	//计算12345678900987654321*98765432100123456789
	mpz_init_set_str(b, "12345678900987654321.21", 10);//10进制 
	mpz_init_set_str(c, "98765432100123456789.12", 10);
	mpz_mul(d, b, c);
	gmp_printf("d = %Zd\n", d);
	mpz_clear(a);
	mpz_clear(b);
	mpz_clear(c);
	mpz_clear(d);

	mpz_t s, ss, sss;
	mpz_init(s);
	mpz_init(ss);
	mpz_init(sss);
	mpz_set_str(s, "11", 10);
	mpz_set_str(ss, "-2", 10);
	mpz_div(sss, s, ss);
	gmp_printf("sd = %Z\n", sss);
	char sdfsd[1024] = {};
	gmp_sprintf(sdfsd, "%F", sss);
	//mp_exp_t exp;
	mpz_get_str(sdfsd, 10,sss);

	int xs = mpz_cmp(sss, ss);

	int ass = 11 / -2;
	getchar();
	return 0;
}