#include "SolutionInfo.h"

SolutionInfo::SolutionInfo()
{
	m_useType = STRATEGY_INIT;
	m_fund = nullptr;
	m_isObserve = false;
	m_filterStock = nullptr;
}

SolutionInfo::~SolutionInfo()
{

}

void SolutionInfo::setParam(StrategyType useType)
{
	m_useType = useType;
}

std::shared_ptr<StrategyInfo> SolutionInfo::strategyInfo(const std::string& stock, StrategyType useType)
{
	auto itStrategyMap = m_strategyAllInfo.find(stock);
	if (itStrategyMap == m_strategyAllInfo.end())
	{
		return std::shared_ptr<StrategyInfo>();
	}
	auto itUseStrategy = itStrategyMap->second.find(useType);
	if (itUseStrategy == itStrategyMap->second.end())
	{
		return std::shared_ptr<StrategyInfo>();
	}
	return itUseStrategy->second.first;
}

std::shared_ptr<StrategyInfo> SolutionInfo::strategyInfoCount(const std::string& stock, StrategyType useType)
{
	auto itStrategyMap = m_strategyAllInfo.find(stock);
	if (itStrategyMap == m_strategyAllInfo.end())
	{
		return std::shared_ptr<StrategyInfo>();
	}
	auto itUseStrategy = itStrategyMap->second.find(useType);
	if (itUseStrategy == itStrategyMap->second.end())
	{
		return std::shared_ptr<StrategyInfo>();
	}
	return itUseStrategy->second.second;
}