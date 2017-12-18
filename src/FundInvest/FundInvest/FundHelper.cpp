#include "stdafx.h"
#include "FundHelper.h"
#include "CStringManager/CStringManagerAPI.h"

std::map<int32_t, std::string> FundHelper::s_mapData;

double FundHelper::Square(const double& x)
{
	return x * x;
}

std::string FundHelper::DoubleToString(const double& x)
{
	return CStringManager::Format("%e", x);
}

std::string FundHelper::BoolToString(const bool& x)
{
	return CStringManager::Format("%d", x);
}

std::string FundHelper::TimeToString(const IntDateTime& time)
{
	std::string result = time.toString();
	int i = 9;
	while (i-- != 0)
	{
		result.pop_back();
	}
	return result;
}

std::string FundHelper::GetInfo(int32_t infoNum)
{
	return s_mapData[infoNum];
}

void FundHelper::init()
{
	s_mapData[DAY_CHG] = "day_chg";
	s_mapData[TIME] = "time";
	s_mapData[UNIT_NET_VALUE] = "unit_net_value";
	s_mapData[ALL_NET_VALUE] = "all_net_value";
	s_mapData[FUND_NAME] = "fund_name";
	s_mapData[IS_OPEN_BUY] = "is_open_buy";
	s_mapData[IS_OPEN_SELL] = "is_open_sell";
	s_mapData[FORECAST1] = "forecast_1";
	s_mapData[FORECAST2] = "forecast_2";
	s_mapData[FORECAST3] = "forecast_3";
	s_mapData[UPDOWN] = "up_down";

    s_mapData[UPDOWN_3] = "up_down_3";

    s_mapData[ALWAYS_15DAYS_3] = "always_15days_3";
    s_mapData[HIGHEST_15DAYS_3] = "highest_15days_3";
    s_mapData[BID_15DAYS_3] = "bid_15days_3";
    s_mapData[SELL_15DAYS_3] = "sell_15days_3";
    
    s_mapData[ALWAYS_30DAYS_3] = "always_30days_3";
    s_mapData[HIGHEST_30DAYS_3] = "highest_30days_3";
    s_mapData[BID_30DAYS_3] = "bid_30days_3";
    s_mapData[SELL_30DAYS_3] = "sell_30days_3";
    
    s_mapData[ALWAYS_90DAYS_3] = "always_90days_3";
    s_mapData[HIGHEST_90DAYS_3] = "highest_90days_3";
    s_mapData[BID_90DAYS_3] = "bid_90days_3";
    s_mapData[SELL_90DAYS_3] = "sell_90days_3";
    
    s_mapData[UPDOWN_5] = "up_down_5";

    s_mapData[ALWAYS_15DAYS_5] = "always_15days_5";
    s_mapData[HIGHEST_15DAYS_5] = "highest_15days_5";
    s_mapData[BID_15DAYS_5] = "bid_15days_5";
    s_mapData[SELL_15DAYS_5] = "sell_15days_5";
    
    s_mapData[ALWAYS_30DAYS_5] = "always_30days_5";
    s_mapData[HIGHEST_30DAYS_5] = "highest_30days_5";
    s_mapData[BID_30DAYS_5] = "bid_30days_5";
    s_mapData[SELL_30DAYS_5] = "sell_30days_5";
    
    s_mapData[ALWAYS_90DAYS_5] = "always_90days_5";
    s_mapData[HIGHEST_90DAYS_5] = "highest_90days_5";
    s_mapData[BID_90DAYS_5] = "bid_90days_5";
    s_mapData[SELL_90DAYS_5] = "sell_90days_5";
        
}