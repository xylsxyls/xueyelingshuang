#include "Solution.h"
#include "SolutionInfo.h"

void Solution::init(const std::map<StrategyType, std::pair<std::shared_ptr<Strategy>, std::shared_ptr<Strategy>>>& vecStrategy)
{
	m_mapStrategy = vecStrategy;
}

void Solution::setParam(StrategyType useType)
{
	m_useType = useType;
}

bool Solution::buy(std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>& buyStock, const IntDateTime& date, const std::shared_ptr<SolutionInfo>& solutionInfo)
{
	return false;
}

bool Solution::sell(std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>& sellStock, const IntDateTime& date, const std::shared_ptr<SolutionInfo>& solutionInfo)
{
	return false;
}

SolutionType Solution::type()
{
	return m_solutionType;
}

std::shared_ptr<Strategy> Solution::strategy(StrategyType useType)
{
	auto itUseType = m_mapStrategy.find(useType);
	if (itUseType == m_mapStrategy.end())
	{
		return std::shared_ptr<Strategy>();
	}
	return itUseType->second.first;
}

std::shared_ptr<Strategy> Solution::strategyCount(StrategyType useType)
{
	auto itUseType = m_mapStrategy.find(useType);
	if (itUseType == m_mapStrategy.end())
	{
		return std::shared_ptr<Strategy>();
	}
	return itUseType->second.second;
}

int32_t Solution::minPollSize(const std::shared_ptr<SolutionInfo>& solutionInfo)
{
	if (solutionInfo->m_strategyAllInfo.empty())
	{
		return 0;
	}
	auto& strategyMap = solutionInfo->m_strategyAllInfo.begin()->second;
	auto itUseStrategy = strategyMap.find(solutionInfo->m_useType);
	if (itUseStrategy == strategyMap.end())
	{
		RCSend("未发现使用类型，minPollSize");
		return 0;
	}
	return itUseStrategy->second.second->minPollSize();
}

int32_t Solution::popSize(int32_t buySize, const std::shared_ptr<SolutionInfo>& solutionInfo)
{
	if (solutionInfo->m_strategyAllInfo.empty())
	{
		return 0;
	}
	auto& strategyMap = solutionInfo->m_strategyAllInfo.begin()->second;
	auto itUseStrategy = strategyMap.find(solutionInfo->m_useType);
	if (itUseStrategy == strategyMap.end())
	{
		RCSend("未发现使用类型，popSize");
		return 0;
	}
	return itUseStrategy->second.second->popSize(buySize);
}

int32_t Solution::stockNum(const std::shared_ptr<SolutionInfo>& solutionInfo)
{
	if (solutionInfo->m_strategyAllInfo.empty())
	{
		return 0;
	}
	auto& strategyMap = solutionInfo->m_strategyAllInfo.begin()->second;
	auto itUseStrategy = strategyMap.find(solutionInfo->m_useType);
	if (itUseStrategy == strategyMap.end())
	{
		RCSend("未发现使用类型，minPollSize");
		return 0;
	}
	return itUseStrategy->second.second->stockNum();
}