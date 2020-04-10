#include "SolutionInfo.h"

SolutionInfo::SolutionInfo()
{
	m_fund = nullptr;
	m_filterStock = nullptr;
}

SolutionInfo::~SolutionInfo()
{

}

std::shared_ptr<Strategy> SolutionInfo::strategy(StrategyType useType)
{
	if (useType == STRATEGY_INIT)
	{
		useType = m_chooseParam.m_useType;
	}
	std::map<StrategyType, StrategyStruct>::iterator itUseStrategy = m_allStrategy.find(useType);
	if (itUseStrategy == m_allStrategy.end())
	{
		return std::shared_ptr<Strategy>();
	}
	return itUseStrategy->second.m_strategy;
}

std::shared_ptr<Strategy> SolutionInfo::strategyCount(StrategyType useType)
{
	if (useType == STRATEGY_INIT)
	{
		useType = m_chooseParam.m_useType;
	}
	std::map<StrategyType, StrategyStruct>::iterator itUseStrategy = m_allStrategy.find(useType);
	if (itUseStrategy == m_allStrategy.end())
	{
		return std::shared_ptr<Strategy>();
	}
	return itUseStrategy->second.m_strategyCount;
}

std::shared_ptr<StrategyInfo> SolutionInfo::strategyInfo(StrategyType useType, const std::string& stock)
{
	std::map<std::string, std::map<StrategyType, StrategyInfoStruct>>::iterator itStrategyMap = m_allStrategyInfo.find(stock);
	if (stock.empty())
	{
		itStrategyMap = m_allStrategyInfo.begin();
	}
	else
	{
		itStrategyMap = m_allStrategyInfo.find(stock);
	}

	if (itStrategyMap == m_allStrategyInfo.end())
	{
		return std::shared_ptr<StrategyInfo>();
	}
	if (useType == STRATEGY_INIT)
	{
		useType = m_chooseParam.m_useType;
	}
	auto itUseStrategy = itStrategyMap->second.find(useType);
	if (itUseStrategy == itStrategyMap->second.end())
	{
		return std::shared_ptr<StrategyInfo>();
	}
	return itUseStrategy->second.m_strategyInfo;
}

std::shared_ptr<StrategyInfo> SolutionInfo::strategyCountInfo(StrategyType useType, const std::string& stock)
{
	std::map<std::string, std::map<StrategyType, StrategyInfoStruct>>::iterator itStrategyMap = m_allStrategyInfo.find(stock);
	if (stock.empty())
	{
		itStrategyMap = m_allStrategyInfo.begin();
	}
	else
	{
		itStrategyMap = m_allStrategyInfo.find(stock);
	}

	if (itStrategyMap == m_allStrategyInfo.end())
	{
		return std::shared_ptr<StrategyInfo>();
	}
	if (useType == STRATEGY_INIT)
	{
		useType = m_chooseParam.m_useType;
	}
	auto itUseStrategy = itStrategyMap->second.find(useType);
	if (itUseStrategy == itStrategyMap->second.end())
	{
		return std::shared_ptr<StrategyInfo>();
	}
	return itUseStrategy->second.m_strategyCountInfo;
}