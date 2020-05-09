#include "SarRiseBackThirtyLineInfo.h"

int32_t SarRiseBackThirtyLineInfo::minPollSize()
{
	return 5;
}

int32_t SarRiseBackThirtyLineInfo::popSize(int32_t buySize)
{
	return buySize - buySize / 2;
}

int32_t SarRiseBackThirtyLineInfo::stockNum()
{
	return 4;
}

std::set<std::string> SarRiseBackThirtyLineInfo::needIndicator()
{
	std::set<std::string> result;
	result.insert("sar");
	result.insert("boll");
	return result;
}

StockLoadInfo SarRiseBackThirtyLineInfo::needStockLoadInfo()
{
	StockLoadInfo result;
	result.m_filterType = RISE_UP;
	result.m_isDislodge688 = true;
	result.m_isDislodgeLiftBan = true;
	return result;
}

int32_t SarRiseBackThirtyLineInfo::needMoveDay()
{
	return 31 + 2;
}