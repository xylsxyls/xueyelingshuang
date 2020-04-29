#include "T0Info.h"

int32_t T0Info::minPollSize()
{
	return 1;
}

int32_t T0Info::popSize(int32_t buySize)
{
	return 0;
}

int32_t T0Info::stockNum()
{
	return 1;
}

std::set<std::string> T0Info::needIndicator()
{
	std::set<std::string> result;
	return result;
}

int32_t T0Info::needMoveDay()
{
	return 2;
}