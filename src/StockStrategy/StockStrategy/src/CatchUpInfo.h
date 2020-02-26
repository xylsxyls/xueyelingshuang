#pragma once
#include "StrategyInfo.h"
#include <memory>

class StockSarIndicator;
class StockBollIndicator;

struct StockStrategyAPI CatchUpInfo : public StrategyInfo
{
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::shared_ptr<StockBollIndicator> m_spBollIndicator;
#ifdef _MSC_VER
#pragma warning(pop)
#endif

	CatchUpInfo()
	{
		m_minPollSize = 2;
	}

	virtual int32_t popSize(int32_t buySize)
	{
		return 0;
	}
};