#include "BigNumberBase.h"
#include "CStringManager/CStringManagerAPI.h"
#include "gmp.h"
#pragma comment(lib, "libgmp.a")
#pragma comment(lib, "libgcc.a")
#pragma comment(lib, "libmingwex.a")
#pragma comment(lib, "libmsvcrt.a")

BigNumberBase::BigNumberBase():
m_prec(0)
{
	mpz_init(m_integer);
}

BigNumberBase::~BigNumberBase()
{
	mpz_clear(m_integer);
}

BigNumberBase::BigNumberBase(int32_t num)
{
	mpz_init_set_ui(m_integer, num);
}

BigNumberBase::BigNumberBase(const char* num)
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

BigNumberBase::BigNumberBase(const BigNumberBase& num)
{
	mpz_init_set(m_integer, num.m_integer);
	m_prec = num.m_prec;
}

void BigNumberBase::TenExp(mpz_t& num, int32_t exp)
{
	char* szExp = new char[exp + 2];
	memset(szExp, 48, exp + 1);
	szExp[exp + 1] = 0;
	szExp[0] = 49;
	mpz_t mpzExp;
	mpz_init_set_str(mpzExp, szExp, 10);
	mpz_mul(num, num, mpzExp);
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

std::string BigNumberBase::toString()
{
	int32_t count = mpz_sizeinbase(m_integer, 2);
	char* num = (char*)::calloc(count + 1, sizeof(char));
	memset(num, 0, count + 1);
	mpz_get_str(num, 10, m_integer);
	std::string result = num;
	::free(num);
	return result;
}

#include <string>
#include <iostream>

int main()
{
	BigNumberBase aaaaa = 5000000;
	BigNumberBase bbbbb = 200000;
	std::string str = (aaaaa - bbbbb).toString();
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
	mp_exp_t exp;
	mpz_get_str(sdfsd, 10,sss);

	int xs = mpz_cmp(sss, ss);

	int ass = 11 / -2;
	getchar();
	return 0;
}