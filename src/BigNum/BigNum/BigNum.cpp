#include "BigNum.h"
#include <cstring>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "CStringManager/CStringManagerAPI.h"
#include <cassert>

BigNum::BigNum()
{

}

BigNum::BigNum(int32_t num)
{
	*this = num;
}

BigNum::BigNum(const char* num)
{
	*this = num;
}

bool BigNum::Check(const char* num)
{
	std::string strNum = num;
	//如果有负号，负号在最前面
	int32_t minusNum = CStringManager::Replace(strNum, "-", "-");
	if (minusNum == 1)
	{
		if (strNum[0] != '-')
		{
			return false;
		}
	}
	else if (minusNum > 1)
	{
		return false;
	}
	//如果有小数点，小数点只有一个
	int32_t decimalsPoint = CStringManager::Replace(strNum, ".", ".");
	if (decimalsPoint > 1)
	{
		return false;
	}
	//如果有出现不是0-9和-.的就判定为错误
	int32_t index = -1;
	while (index++ != strNum.size() - 1)
	{
		if (!((strNum[index] <= '9' && strNum[index] >= '0') || 
			(strNum[index] == '-') || 
			(strNum[index] == '.')))
		{
			return false;
		}
	}
	return true;
}

BigNum BigNum::operator = (int32_t num)
{
	m_listIntegerNum.clear();
	m_listDecimalsNum.clear();
	m_isMinus = (num < 0);
	IntToList(num, m_listIntegerNum);
	return *this;
}

BigNum BigNum::operator = (const char* num)
{
	if (Check(num) == false)
	{
		assert(0);
	}
	m_listIntegerNum.clear();
	m_listDecimalsNum.clear();
	std::string strNum = num;
	if (strNum[0] == '-')
	{
		m_isMinus = true;
		strNum.erase(strNum.begin());
	}

	std::vector<std::string> vecNum = CStringManager::split(strNum, ".");
	if (vecNum.size() == 1)
	{
		StringToList(vecNum[0], m_listIntegerNum, true);
	}
	else
	{
		StringToList(vecNum[0], m_listIntegerNum, true);
		StringToList(vecNum[1], m_listDecimalsNum, false);
	}

	if ((m_listIntegerNum.size() == 1) && 
		(m_listIntegerNum.front() == 0) && 
		(m_isMinus == true) && 
		(m_listDecimalsNum.empty() == true))
	{
		m_isMinus = false;
	}
	return *this;
}

BigNum operator + (const BigNum& x, const BigNum& y)
{
	bool xFlag = x.IsMinus();
	bool yFlag = y.IsMinus();

	BigNum result;
	result.m_listIntegerNum.push_back(x.m_listIntegerNum.back() + y.m_listIntegerNum.back());
	int32_t xNumSize = x.m_listDecimalsNum.size();
	int32_t yNumSize = y.m_listDecimalsNum.size();
	if (xNumSize >= yNumSize)
	{

	}

	return BigNum();
}

BigNum operator - (const BigNum& x, const BigNum& y)
{
	return BigNum();
}

BigNum operator * (const BigNum& x, const BigNum& y)
{
	return BigNum();
}

BigNum operator / (const BigNum& x, const BigNum& y)
{
	return BigNum();
}

BigNum operator % (const BigNum& x, const BigNum& y)
{
	return BigNum();
}

int32_t operator == (BigNum x, BigNum y)
{
	return 0;
}

int32_t operator != (BigNum x, BigNum y)
{
	return 0;
}

int32_t operator >  (BigNum x, BigNum y)
{
	return 0;
}

int32_t operator >= (BigNum x, BigNum y)
{
	return 0;
}

int32_t operator <  (BigNum x, BigNum y)
{
	return 0;
}

int32_t operator <= (BigNum x, BigNum y)
{
	return 0;
}

BigNum BigNum::operator ++ ()
{
	return BigNum();
}

BigNum BigNum::operator -- ()
{
	return BigNum();
}

BigNum BigNum::operator ++ (int)
{
	return BigNum();
}

BigNum BigNum::operator -- (int)
{
	return BigNum();
}

std::string BigNum::toString()
{
	return std::string();
}

std::string BigNum::IntegerToString()
{
	return std::string();
}

std::string BigNum::DecimalsToString()
{
	return std::string();
}

bool BigNum::IsMinus() const
{
	return m_isMinus;
}

void BigNum::ChangeFlag()
{
	m_isMinus = !m_isMinus;
}

std::string BigNum::toRadixString(int32_t radix, const std::map<int32_t, std::string>& radixMap)
{
	return std::string();
}

void BigNum::IntToList(int32_t num, std::list<int32_t>& listNum)
{
	while (num)
	{
		listNum.push_back(num % 10);
		num = num / 10;
	}
}

void BigNum::StringToList(const std::string& num, std::list<int32_t>& listNum, bool isInteger)
{
	int32_t index = -1;
	while (index++ != num.size() - 1)
	{
		if (isInteger)
		{
			if (listNum.empty() && num[index] == '0')
			{
				continue;
			}
		}
		listNum.push_back(num[index] - 48);
	}
	if (listNum.empty() && isInteger)
	{
		listNum.push_back(0);
		return;
	}
	else if (isInteger == false)
	{
		while (listNum.size() > 0 && (listNum.back() == 0))
		{
			listNum.pop_back();
		}
	}
}
	
int main()
{
	BigNum s = "-.";
	getchar();
	return 0;
}