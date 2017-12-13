#pragma once
#include <string>
#include <stdint.h>
#include <map>
#include "IntDateTime/IntDateTimeAPI.h"

#define GETINFO FundHelper::GetInfo

enum
{
	DAY_CHG,
	TIME,
	UNIT_NET_VALUE,
	ALL_NET_VALUE,
	FUND_NAME,
	IS_OPEN_BUY,
	IS_OPEN_SELL,
	FORECAST1,
	FORECAST2,
	FORECAST3,
	UPDOWN,
	UPDOWNWEEK
};

class FundHelper
{
public:
	//Æ½·½
	static double Square(const double& x);

	static std::string DoubleToString(const double& x);

	static std::string BoolToString(const bool& x);

	static std::string TimeToString(const IntDateTime& time);

	static std::string GetInfo(int32_t infoNum);

	static void init();

	static std::map<int32_t, std::string> s_mapData;

};