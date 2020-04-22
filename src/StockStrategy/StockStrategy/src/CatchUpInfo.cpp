#include "CatchUpInfo.h"

int32_t CatchUpInfo::minPollSize()
{
	return 2;
}

int32_t CatchUpInfo::popSize(int32_t buySize)
{
	return 0;
}

int32_t CatchUpInfo::stockNum()
{
	return 2;
}

std::set<std::string> CatchUpInfo::needIndicator()
{
	std::set<std::string> result;
	result.insert("boll");
	return result;
}

int32_t CatchUpInfo::needMoveDay()
{
	return 2 + 2;
}