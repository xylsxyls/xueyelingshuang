#pragma once
#include <string>
#include <stdint.h>
#include <map>

#define GETINFO FundHelper::GetInfo

enum
{
	DAY_CHG,
	TIME,
	UNIT_NET_VALUE,
	ALL_NET_VALUE,
	FUND_NAME,
	IS_OPEN_BUY,
	IS_OPEN_SELL
};

class FundHelper
{
public:
	//Æ½·½
	static double Square(const double& x);

	static std::string DoubleToString(const double& x);

	static std::string BoolToString(const bool& x);

	static std::string GetInfo(int32_t infoNum);

	static void init();

	static std::map<int32_t, std::string> s_mapData;

};