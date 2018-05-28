#ifndef _BIGNUMBER_H__
#define _BIGNUMBER_H__
#include "BigNumber.h"

BigNumber operator + (const BigNumber& x, const BigNumber& y)
{
	return BigNumber::add(x, y);
}

BigNumber operator - (const BigNumber& x, const BigNumber& y)
{
	return BigNumber::sub(x, y);
}

BigNumber operator * (const BigNumber& x, const BigNumber& y)
{
	return BigNumber::mul(x, y);
}

BigNumber operator / (const BigNumber& x, const BigNumber& y)
{
	return BigNumber::div(x, y);
}

BigNumber operator % (const BigNumber& x, const BigNumber& y)
{
	return BigNumber::mod(x, y);
}

bool operator == (const BigNumber& x, const BigNumber& y)
{
	return BigNumber::equal(x, y);
}

bool operator != (const BigNumber& x, const BigNumber& y)
{
	return BigNumber::notEqual(x, y);
}

bool operator > (const BigNumber& x, const BigNumber& y)
{
	return BigNumber::big(x, y);
}

bool operator >= (const BigNumber& x, const BigNumber& y)
{
	return BigNumber::bigEqual(x, y);
}

bool operator < (const BigNumber& x, const BigNumber& y)
{
	return BigNumber::small(x, y);
}

bool operator <= (const BigNumber& x, const BigNumber& y)
{
	return BigNumber::smallEqual(x, y);
}

#endif