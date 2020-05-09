#include "SarRiseBackInfo.h"

int32_t SarRiseBackInfo::minPollSize()
{
	return 5;
}

int32_t SarRiseBackInfo::popSize(int32_t buySize)
{
	return buySize - buySize / 2;
}

int32_t SarRiseBackInfo::stockNum()
{
	return 4;
}

std::set<std::string> SarRiseBackInfo::needIndicator()
{
	std::set<std::string> result;
	result.insert("sar");
	result.insert("boll");
	return result;
}

StockLoadInfo SarRiseBackInfo::needStockLoadInfo()
{
	StockLoadInfo result;
	result.m_filterType = RISE_UP;
	result.m_isDislodge688 = true;
	result.m_isDislodgeLiftBan = true;
	return result;
}

int32_t SarRiseBackInfo::needMoveDay()
{
	return 2;
}