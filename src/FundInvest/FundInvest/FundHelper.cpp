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
}