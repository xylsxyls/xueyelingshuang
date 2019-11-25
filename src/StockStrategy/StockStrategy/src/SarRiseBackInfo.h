#pragma once
#include "StrategyInfo.h"
#include <memory>

class StockSarIndicator;
class StockBollIndicator;

struct StockStrategyAPI SarRiseBackInfo : public StrategyInfo
{
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::shared_ptr<StockSarIndicator> m_spSarIndicator;
	std::shared_ptr<StockBollIndicator> m_spBollIndicator;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};