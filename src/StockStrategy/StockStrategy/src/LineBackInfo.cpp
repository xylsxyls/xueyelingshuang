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