#include "ReconstructionInfo.h"

int32_t ReconstructionInfo::minPollSize()
{
	return 2;
}

int32_t ReconstructionInfo::popSize(int32_t buySize)
{
	return 0;
}

int32_t ReconstructionInfo::stockNum()
{
	return 2;
}

std::set<std::string> ReconstructionInfo::needIndicator()
{
	std::set<std::string> result;
	result.insert("boll");
	return result;
}

StockLoadInfo ReconstructionInfo::needStockLoadInfo()
{
	StockLoadInfo result;
	result.m_filterType = RISE_UP;
	result.m_isDislodge688 = true;
	result.m_isDislodgeLiftBan = true;
	return result;
}

int32_t ReconstructionInfo::needMoveDay()
{
	return 2 + 2;
}