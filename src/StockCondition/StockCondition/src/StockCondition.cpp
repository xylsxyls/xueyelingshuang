#include "StockCondition.h"
#include "StockWrRsi.h"

StockCondition::StockCondition()
{

}

StockCondition::~StockCondition()
{
	destroyAll();
}

StockCondition& StockCondition::instance()
{
	static StockCondition s_stockCondition;
	return s_stockCondition;
}

Strategy* StockCondition::getStrategy(ConditionEnum condition)
{
	auto itStrategy = m_strategy.find(condition);
	if (itStrategy != m_strategy.end())
	{
		return itStrategy->second;
	}

	Strategy* strategy = nullptr;
	switch (condition)
	{
	case StockCondition::WR_RSI:
	{
		strategy = new StockWrRsi;
		m_strategy[StockCondition::WR_RSI] = strategy;
	}
	break;
	default:
		break;
	}
	return strategy;
}

void StockCondition::destroyAll()
{
	for (auto itStrategy = m_strategy.begin(); itStrategy != m_strategy.end(); ++itStrategy)
	{
		delete itStrategy->second;
	}
}