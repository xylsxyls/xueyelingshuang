#include "FastNumber.h"
#include "CStringManager/CStringManagerAPI.h"
#include <math.h>

//num*10µÄexp´Î·½
static void TenExp(int64_t& num, int32_t exp)
{
	if (exp == 0)
	{
		return;
	}
	while (exp-- != 0)
	{
		num *= 10;
	}
}

FastNumber::FastNumber() :
m_divFlag(HALF_ADJUST),
m_base(0),
m_prec(0),
m_isZero(false)
{
#ifdef _DEBUG
	m_num = toString();
#endif
}

FastNumber::FastNumber(int32_t num) :
m_divFlag(HALF_ADJUST),
m_prec(0),
m_isZero(false)
{
	m_base = num;
#ifdef _DEBUG
	m_num = toString();
#endif
}

FastNumber::FastNumber(const char* num) :
m_divFlag(HALF_ADJUST),
m_isZero(false)
{
	std::vector<std::string> vecNum;
	CStringManager::split(vecNum, num, '.');
	auto size = vecNum.size();
	if (size == 1)
	{
		m_prec = 0;
		m_base = CStringManager::atoi64(num);
	}
	else if (size == 2)
	{
		m_prec = vecNum[1].size();
		std::string strNum = num;
		CStringManager::Replace(strNum, ".", "");
		m_base = CStringManager::atoi64(strNum.c_str());
		if (m_prec > 9)
		{
			setPrec(9);
		}
	}
	else
	{
		m_prec = 0;
		m_base = 0;
	}
#ifdef _DEBUG
	m_num = toString();
#endif
}

FastNumber::FastNumber(double num)
{
	*this = CStringManager::Format("%lf", num).c_str();
}

FastNumber::FastNumber(const FastNumber& num) :
m_divFlag(HALF_ADJUST),
m_isZero(num.m_isZero)
{
	m_base = num.m_base;
	m_prec = num.m_prec;
#ifdef _DEBUG
	m_num = toString();
#endif
}

FastNumber FastNumber::operator=(const FastNumber& num)
{
	m_base = num.m_base;
	m_prec = num.m_prec;
	m_divFlag = HALF_ADJUST;
	m_isZero = num.m_isZero;
#ifdef _DEBUG
	m_num = toString();
#endif
	return *this;
}

FastNumber::~FastNumber()
{
	
}

FastNumber FastNumber::operator=(double num)
{
	*this = CStringManager::Format("%lf", num).c_str();
	m_divFlag = HALF_ADJUST;
	m_isZero = false;
#ifdef _DEBUG
	m_num = toString();
#endif
	return *this;
}

FastNumber FastNumber::operator=(const char* num)
{
	std::vector<std::string> vecNum;
	CStringManager::split(vecNum, num, '.');
	auto size = vecNum.size();
	if (size == 1)
	{
		m_prec = 0;
		m_base = CStringManager::atoi64(num);
	}
	else if (size == 2)
	{
		m_prec = vecNum[1].size();
		std::string strNum = num;
		CStringManager::Replace(strNum, ".", "");
		m_base = CStringManager::atoi64(strNum.c_str());
		if (m_prec > 9)
		{
			setPrec(9);
		}
	}
	else
	{
		m_prec = 0;
		m_base = 0;
	}
	m_divFlag = HALF_ADJUST;
	m_isZero = false;
#ifdef _DEBUG
	m_num = toString();
#endif
	return *this;
}

FastNumber FastNumber::operator=(int32_t num)
{
	m_base = num;
	m_prec = 0;
	m_divFlag = HALF_ADJUST;
	m_isZero = false;
#ifdef _DEBUG
	m_num = toString();
#endif
	return *this;
}

FastNumber FastNumber::operator++ ()
{
	*this = *this + 1;
#ifdef _DEBUG
	m_num = toString();
#endif
	return *this;
}

FastNumber FastNumber::operator -- ()
{
	*this = *this - 1;
#ifdef _DEBUG
	m_num = toString();
#endif
	return *this;
}

FastNumber FastNumber::pow(const FastNumber& num)
{
	return ::pow(atof(toString().c_str()), atof(num.toString().c_str()));
}

FastNumber FastNumber::operator ++ (int)
{
	FastNumber result = *this;
	*this = *this + 1;
#ifdef _DEBUG
	m_num = toString();
#endif
	return result;
}

FastNumber FastNumber::operator -- (int)
{
	FastNumber result = *this;
	*this = *this - 1;
#ifdef _DEBUG
	m_num = toString();
#endif
	return result;
}

std::string FastNumber::toString() const
{
	std::string result;
#ifdef _MSC_VER
	CStringManager::Format(result, "%I64d", m_base);
#elif __unix__
	CStringManager::Format(result, "%lld", m_base);
#endif
	if (m_prec == 0)
	{
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
	return result;
}

FastNumber FastNumber::zero() const
{
	FastNumber result = *this;
	result.m_isZero = true;
	return result;
}

FastNumber FastNumber::abs() const
{
	FastNumber result = *this;
	if (result < 0)
	{
		return result * -1;
	}
	return result;
}

FastNumber FastNumber::toPrec(int32_t prec, PrecFlag flag) const
{
	FastNumber result = *this;
	if (prec > 9)
	{
		prec = 9;
	}
	result.setPrec(prec, flag);
	return result;
}

FastNumber& FastNumber::setPrec(int32_t prec, PrecFlag flag)
{
	if (prec == -1 || prec > 9)
	{
		prec = 9;
	}
	if (m_prec == prec)
	{
		return *this;
	}
	if (m_prec < prec)
	{
		TenExp(m_base, (prec - m_prec));
		m_prec = prec;
#ifdef _DEBUG
		m_num = toString();
#endif
		return *this;
	}
	if (flag == FastNumber::HALF_ADJUST)
	{
		std::string result = toString();
		if (result[result.size() - (m_prec - prec)] < '5')
		{
			setPrec(prec, FastNumber::ROUND_OFF);
#ifdef _DEBUG
			m_num = toString();
#endif
			return *this;
		}

		if (FastNumber::Compare(*this, "0") == BigNumberCompare::BIG)
		{
			setPrec(prec, FastNumber::ROUND_UP);
#ifdef _DEBUG
			m_num = toString();
#endif
			return *this;
		}
		setPrec(prec, FastNumber::ROUND_DOWN);
#ifdef _DEBUG
		m_num = toString();
#endif
		return *this;
	}

	int32_t exp = m_prec - prec;
	int64_t expNum = 1;
	TenExp(expNum, exp);

	switch (flag)
	{
	case FastNumber::ROUND_OFF:
	{
		m_base /= expNum;
		break;
	}
	case FastNumber::ROUND_UP:
	{
		m_base /= expNum;
		++m_base;
		break;
	}
	case FastNumber::ROUND_DOWN:
	{
		m_base /= expNum;
		--m_base;
		break;
	}
	default:
		break;
	}

	m_prec = prec;
#ifdef _DEBUG
	m_num = toString();
#endif
	return *this;
}

FastNumber FastNumber::minNumber(const FastNumber& x, const FastNumber& y)
{
	return x < y ? x : y;
}

FastNumber FastNumber::maxNumber(const FastNumber& x, const FastNumber& y)
{
	return x > y ? x : y;
}

FastNumber FastNumber::add(const FastNumber& x, const FastNumber& y)
{
	FastNumber result;
	if (x.m_prec == y.m_prec)
	{
		result.m_base = x.m_base + y.m_base;
		result.m_prec = x.m_prec;
	}
	else if (x.m_prec < y.m_prec)
	{
		result = x;
		TenExp(result.m_base, y.m_prec - x.m_prec);
		result.m_prec = y.m_prec;
		result.m_base += y.m_base;
	}
	else
	{
		result = y;
		TenExp(result.m_base, x.m_prec - y.m_prec);
		result.m_prec = x.m_prec;
		result.m_base += x.m_base;
	}
	return result;
}

FastNumber FastNumber::sub(const FastNumber& x, const FastNumber& y)
{
	FastNumber result;
	if (x.m_prec == y.m_prec)
	{
		result.m_prec = x.m_prec;
		result.m_base = x.m_base - y.m_base;
	}
	else if (x.m_prec < y.m_prec)
	{
		result = x;
		TenExp(result.m_base, y.m_prec - x.m_prec);
		result.m_prec = y.m_prec;
		result.m_base -= y.m_base;
	}
	else
	{
		result = y;
		TenExp(result.m_base, x.m_prec - y.m_prec);
		result.m_prec = x.m_prec;
		result.m_base = x.m_base - result.m_base;
	}
	return result;
}

FastNumber FastNumber::mul(const FastNumber& x, const FastNumber& y)
{
	FastNumber result;
	result.m_prec = x.m_prec + y.m_prec;
	result.m_base = x.m_base * y.m_base;
	if (result.m_prec > 9)
	{
		result.setPrec(9);
		//return result;
	}
	//int64_t xTemp = x.m_base;
	//int64_t yTemp = y.m_base;
	//int32_t xCount = (result.m_prec - 9) / 2;
	//int32_t yCount = result.m_prec - 9 - xCount;
	//while (xCount-- != 0)
	//{
	//	xTemp /= 10;
	//}
	//while (yCount-- != 0)
	//{
	//	yTemp /= 10;
	//}
	//result.m_prec = 9;
	//result.m_base = xTemp * yTemp;
	//RCSend("less");
	return result;
}

FastNumber FastNumber::div(const FastNumber& x, const FastNumber& y)
{
	FastNumber result;
	if (y.m_isZero && y == 0)
	{
		return 0;
	}
	if (x.m_prec == 0 && y.m_prec == 0)
	{
		result.m_base = x.m_base / y.m_base;
		return result;
	}

	int64_t dividerTemp = x.m_base;
	int64_t divisorBase = y.m_base;
	int32_t divisorPrec = y.m_prec;
	if (divisorPrec > 4)
	{
		int32_t count = y.m_prec - 4;
		while (count-- != 0)
		{
			divisorBase /= 10;
		}
		divisorPrec = 4;
	}
	TenExp(dividerTemp, 9 - x.m_prec);
	if (divisorBase == 0)
	{
		result.m_base = 0;
	}
	else
	{
		result.m_base = dividerTemp / divisorBase;
	}
	result.m_prec = 9 - divisorPrec;
	return result;
}

FastNumber FastNumber::mod(const FastNumber& x, const FastNumber& y)
{
	return sub(x, mul(div(x, y), y));
}

bool FastNumber::equal(const FastNumber& x, const FastNumber& y)
{
	return FastNumber::Compare(x, y) == FastNumber::EQUAL;
}

bool FastNumber::notEqual(const FastNumber& x, const FastNumber& y)
{
	return FastNumber::Compare(x, y) != FastNumber::EQUAL;
}

bool FastNumber::bigThan(const FastNumber& x, const FastNumber& y)
{
	return FastNumber::Compare(x, y) == FastNumber::BIG;
}

bool FastNumber::bigEqual(const FastNumber& x, const FastNumber& y)
{
	return (FastNumber::Compare(x, y) == FastNumber::BIG) ||
		(FastNumber::Compare(x, y) == FastNumber::EQUAL);
}

bool FastNumber::smallThan(const FastNumber& x, const FastNumber& y)
{
	return FastNumber::Compare(x, y) == FastNumber::SMALL;
}

bool FastNumber::smallEqual(const FastNumber& x, const FastNumber& y)
{
	return (FastNumber::Compare(x, y) == FastNumber::SMALL) ||
		(FastNumber::Compare(x, y) == FastNumber::EQUAL);
}

FastNumber::BigNumberCompare FastNumber::Compare(const FastNumber& x, const FastNumber& y)
{
	FastNumber result = sub(x, y);
	if (result.m_base < 0)
	{
		return BigNumberCompare::SMALL;
	}
	else if (result.m_base > 0)
	{
		return BigNumberCompare::BIG;
	}
	return BigNumberCompare::EQUAL;
}

//#include <string>
//#include <iostream>
//
//int main()
//{
//	FastNumber x = "1.5688753";
//	//x.setPrec(10);
//	FastNumber y = x / x;
//	FastNumber z = x == y;
//
//	RCSend("x = %s,y = %s\n", x.toString().c_str(), y.toString().c_str());
//	RCSend("x + y = %s\n", (x + y).toString().c_str());
//	RCSend("x - y = %s\n", (x - y).toString().c_str());
//	RCSend("x * y = %s\n", (x * y).toString().c_str());
//	RCSend("x / y = %s\n", (x / y).toString().c_str());
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
//			RCSend("x + y = %s\n", (x + y).toString().c_str());
//			RCSend("x - y = %s\n", (x - y).toString().c_str());
//			RCSend("x * y = %s\n", (x * y).toString().c_str());
//			RCSend("x / y = %s\n", (x / y).toString().c_str());
//			if (x == "0")
//			{
//				RCSend("\n");
//				continue;
//			}
//			RCSend("\n");
//		}
//	}
//	getchar();
//	return 0;
//}