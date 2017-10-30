#pragma once
#include <stdint.h>
#include <list>
#include <map>

class BigNum
{
public:
	BigNum();
	BigNum(int32_t num);
	BigNum(const char* num);

public:
	bool Check(const char* num);

public:
	BigNum operator = (int32_t num);
	BigNum operator = (const char* num);

public:
	friend BigNum operator + (const BigNum& x, const BigNum& y);
	friend BigNum operator - (const BigNum& x, const BigNum& y);
	friend BigNum operator * (const BigNum& x, const BigNum& y);
	friend BigNum operator / (const BigNum& x, const BigNum& y);
	friend BigNum operator % (const BigNum& x, const BigNum& y);

public:
	friend int32_t operator == (BigNum x, BigNum y);
	friend int32_t operator != (BigNum x, BigNum y);
	friend int32_t operator >  (BigNum x, BigNum y);
	friend int32_t operator >= (BigNum x, BigNum y);
	friend int32_t operator <  (BigNum x, BigNum y);
	friend int32_t operator <= (BigNum x, BigNum y);

public:
	BigNum operator ++ ();//前++
	BigNum operator -- ();//前--
	BigNum operator ++ (int);//后++
	BigNum operator -- (int);//后--

public:
	//按十进制转化为字符串
	std::string toString();

	//整数部分转化为字符串
	std::string IntegerToString();

	//小数部分转化为字符串
	std::string DecimalsToString();

	//是否为负数
	bool IsMinus() const;

	//将负数改为正数
	void ChangeFlag();

	//按进制输出
	std::string toRadixString(int32_t radix, const std::map<int32_t, std::string>& radixMap);

private:
	void IntToList(int32_t num, std::list<int32_t>& listNum);

	void StringToList(const std::string& num, std::list<int32_t>& listNum, bool isInteger);

public:
	//整数部分
	std::list<int32_t> m_listIntegerNum;

	//小数部分
	std::list<int32_t> m_listDecimalsNum;

	//是否有负号标志
	bool m_isMinus = false;

#ifdef _DEBUG
	std::string outString;
#endif
};