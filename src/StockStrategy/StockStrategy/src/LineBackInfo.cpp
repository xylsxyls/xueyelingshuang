#include "LineBackInfo.h"

int32_t LineBackInfo::minPollSize()
{
	return 5;
}

int32_t LineBackInfo::popSize(int32_t buySize)
{
	return buySize / 2;
}

int32_t LineBackInfo::stockNum()
{
	return 4;
}

std::set<std::string> LineBackInfo::needIndicator()
{
	std::set<std::string> result;
	result.insert("sar");
	result.insert("boll");
	return result;
}

StockLoadInfo LineBackInfo::needStockLoadInfo()
{
	StockLoadInfo result;
	result.m_filterType = RISE_UP;
	result.m_isDislodge688 = true;
	result.m_isDislodgeLiftBan = true;
	return result;
}

int32_t LineBackInfo::needMoveDay()
{
	return 32 + 2;
}