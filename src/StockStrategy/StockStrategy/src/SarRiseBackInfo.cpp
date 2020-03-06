#include "SarRiseBackInfo.h"

int32_t SarRiseBackInfo::minPollSize()
{
	return 5;
}

int32_t SarRiseBackInfo::popSize(int32_t buySize)
{
	return buySize - buySize / 2;
}