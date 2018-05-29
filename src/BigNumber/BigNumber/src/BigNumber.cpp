#include "BigNumber.h"
#include "BigNumberBase/BigNumberBaseAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#pragma comment(lib, "libgmp.a")
#pragma comment(lib, "libgcc.a")
#pragma comment(lib, "libmingwex.a")
#pragma comment(lib, "libmsvcrt.a")

BigNumber::BigNumber():
m_prec(-1),
m_flag(HALF_ADJUST)
{
	m_base = new BigNumberBase;
}

BigNumber::BigNumber(int32_t num):
m_prec(-1),
m_flag(HALF_ADJUST)
{
	m_base = new BigNumberBase;
	*m_base = CStringManager::Format("%d", num).c_str();
}

BigNumber::BigNumber(const char* num):
m_prec(-1),
m_flag(HALF_ADJUST)
{
	m_base = new BigNumberBase;
	*m_base = num;
}

BigNumber::BigNumber(double num):
m_prec(-1),
m_flag(HALF_ADJUST)
{
	m_base = new BigNumberBase;
	*m_base = CStringManager::Format("%lf", num).c_str();
}

BigNumber::BigNumber(const BigNumber& num):
m_prec(-1),
m_flag(HALF_ADJUST)
{
	m_base = new BigNumberBase;
	*m_base = *num.m_base;
}

BigNumber BigNumber::operator=(double num)
{
	*m_base = CStringManager::Format("%lf", num).c_str();
	m_prec = -1;
	m_flag = HALF_ADJUST;
	return *this;
}

BigNumber BigNumber::operator=(const char* num)
{
	*m_base = num;
	m_prec = -1;
	m_flag = HALF_ADJUST;
	return *this;
}

BigNumber BigNumber::operator=(int32_t num)
{
	*m_base = CStringManager::Format("%d", num).c_str();
	m_prec = -1;
	m_flag = HALF_ADJUST;
	return *this;
}

BigNumber BigNumber::operator++ ()
{
	*m_base = BigNumberBase::add(*m_base, "1");
	return *this;
}

BigNumber BigNumber::operator -- ()
{
	*m_base = BigNumberBase::sub(*m_base, "1");
	return *this;
}

BigNumber BigNumber::operator ++ (int)
{
	BigNumber result;
	*result.m_base = *m_base;
	*m_base = BigNumberBase::add(*m_base, "1");
	return result;
}

BigNumber BigNumber::operator -- (int)
{
	BigNumber result;
	*result.m_base = *m_base;
	*m_base = BigNumberBase::sub(*m_base, "1");
	return result;
}

BigNumber BigNumber::pow(const BigNumber& powNum, int32_t prec, PrecFlag flag)
{
	BigNumber result;
	*result.m_base = m_base->pow(*powNum.m_base, prec, (BigNumberBase::PrecFlag)flag);
	return result;
}

std::string BigNumber::toString()
{
	return m_base->toString();
}

void BigNumber::setPrec(int32_t prec, PrecFlag flag)
{
	m_base->setPrec(prec, (BigNumberBase::PrecFlag)flag);
}

void BigNumber::setFixedPrec(int32_t fixedPrec, PrecFlag fixedPrecFlag)
{
	m_base->setFixedPrec(fixedPrec, (BigNumberBase::PrecFlag)fixedPrecFlag);
}

void BigNumber::setDivParam(int32_t prec, PrecFlag flag)
{
	m_prec = prec;
	m_flag = flag;
}

BigNumber BigNumber::add(const BigNumber& x, const BigNumber& y)
{
	BigNumber result;
	*result.m_base = BigNumberBase::add(*x.m_base, *y.m_base);
	return result;
}

BigNumber BigNumber::sub(const BigNumber& x, const BigNumber& y)
{
	BigNumber result;
	*result.m_base = BigNumberBase::sub(*x.m_base, *y.m_base);
	return result;
}

BigNumber BigNumber::mul(const BigNumber& x, const BigNumber& y)
{
	BigNumber result;
	*result.m_base = BigNumberBase::mul(*x.m_base, *y.m_base);
	return result;
}

BigNumber BigNumber::div(const BigNumber& x, const BigNumber& y)
{
	BigNumber result;
	if (x.m_base->m_prec == 0 && y.m_base->m_prec == 0)
	{
		*result.m_base = BigNumberBase::div(*x.m_base, *y.m_base);
		return result;
	}
	*result.m_base = x.m_base->div(*y.m_base, x.m_prec, (BigNumberBase::PrecFlag)x.m_flag);
	return result;
}

BigNumber BigNumber::mod(const BigNumber& x, const BigNumber& y)
{
	BigNumber result;
	if (x.m_base->m_prec == 0 && y.m_base->m_prec == 0)
	{
		*result.m_base = BigNumberBase::mod(*x.m_base, *y.m_base);
		return result;
	}
	return result;
}

bool BigNumber::equal(const BigNumber& x, const BigNumber& y)
{
	return BigNumberBase::Compare(*x.m_base, *y.m_base) == BigNumberBase::EQUAL;
}

bool BigNumber::notEqual(const BigNumber& x, const BigNumber& y)
{
	return BigNumberBase::Compare(*x.m_base, *y.m_base) != BigNumberBase::EQUAL;
}

bool BigNumber::big(const BigNumber& x, const BigNumber& y)
{
	return BigNumberBase::Compare(*x.m_base, *y.m_base) != BigNumberBase::BIG;
}

bool BigNumber::bigEqual(const BigNumber& x, const BigNumber& y)
{
	return (BigNumberBase::Compare(*x.m_base, *y.m_base) != BigNumberBase::BIG) ||
		(BigNumberBase::Compare(*x.m_base, *y.m_base) != BigNumberBase::EQUAL);
}

bool BigNumber::small(const BigNumber& x, const BigNumber& y)
{
	return BigNumberBase::Compare(*x.m_base, *y.m_base) != BigNumberBase::SMALL;
}

bool BigNumber::smallEqual(const BigNumber& x, const BigNumber& y)
{
	return (BigNumberBase::Compare(*x.m_base, *y.m_base) != BigNumberBase::SMALL) ||
		(BigNumberBase::Compare(*x.m_base, *y.m_base) != BigNumberBase::EQUAL);
}

#include <string>
#include <iostream>
#include "D:\\SendToMessageTest.h"

int main()
{
	BigNumber x = "-0.2";
	BigNumber y = "0.4";
	RCSend("x = %s,y = %s\n", x.toString().c_str(), y.toString().c_str());
	RCSend("x + y = %s\n", (x + y).toString().c_str());
	RCSend("x - y = %s\n", (x - y).toString().c_str());
	RCSend("x * y = %s\n", (x * y).toString().c_str());
	RCSend("x / y = %s\n", (x / y).toString().c_str());
	RCSend("x ^ y = %s\n", x.pow(y).toString().c_str());
	RCSend("\n");
	std::vector<std::string> xVec;
	xVec.push_back("0.2");
	xVec.push_back("-0.2");
	xVec.push_back("2.2");
	xVec.push_back("-2.2");
	std::vector<std::string> yVec;
	yVec.push_back("0.2");
	yVec.push_back("-0.2");
	yVec.push_back("2.2");
	yVec.push_back("-2.2");
	for (unsigned int xIndex = 0; xIndex < xVec.size(); xIndex++)
	{
		for (unsigned int yIndex = 0; yIndex < yVec.size(); yIndex++)
		{
			x = xVec[xIndex].c_str();
			y = yVec[yIndex].c_str();
			RCSend("x = %s,y = %s\n", x.toString().c_str(), y.toString().c_str());
			RCSend("x + y = %s\n", (x + y).toString().c_str());
			RCSend("x - y = %s\n", (x - y).toString().c_str());
			RCSend("x * y = %s\n", (x * y).toString().c_str());
			RCSend("x / y = %s\n", (x / y).toString().c_str());
			if (x == "0")
			{
				RCSend("\n");
				continue;
			}
			RCSend("x ^ y = %s\n", x.pow(y).toString().c_str());
			RCSend("\n");
		}
	}
	getchar();
	return 0;
}