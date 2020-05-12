#include "UpwardInfo.h"

int32_t UpwardInfo::minPollSize()
{
	return 2;
}

int32_t UpwardInfo::popSize(int32_t buySize)
{
	return 0;
}

int32_t UpwardInfo::stockNum()
{
	return 2;
}

std::set<std::string> UpwardInfo::needIndicator()
{
	std::set<std::string> result;
	result.insert("boll");
	return result;
}

StockLoadInfo UpwardInfo::needStockLoadInfo()
{
	StockLoadInfo result;
	result.m_filterType = RISE_UP;
	result.m_isDislodge688 = true;
	result.m_isDislodgeLiftBan = true;
	return result;
}

int32_t UpwardInfo::needMoveDay()
{
	return 2 + 2;
}