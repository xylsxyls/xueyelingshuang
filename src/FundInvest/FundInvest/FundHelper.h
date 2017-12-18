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

    UPDOWN_3,

    ALWAYS_15DAYS_3,
    HIGHEST_15DAYS_3,
    BID_15DAYS_3,
    SELL_15DAYS_3,
    
    ALWAYS_30DAYS_3,
    HIGHEST_30DAYS_3,
    BID_30DAYS_3,
    SELL_30DAYS_3,
    
    ALWAYS_90DAYS_3,
    HIGHEST_90DAYS_3,
    BID_90DAYS_3,
    SELL_90DAYS_3,

    UPDOWN_5,

    ALWAYS_15DAYS_5,
    HIGHEST_15DAYS_5,
    BID_15DAYS_5,
    SELL_15DAYS_5,
    
    ALWAYS_30DAYS_5,
    HIGHEST_30DAYS_5,
    BID_30DAYS_5,
    SELL_30DAYS_5,
    
    ALWAYS_90DAYS_5,
    HIGHEST_90DAYS_5,
    BID_90DAYS_5,
    SELL_90DAYS_5,
    
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