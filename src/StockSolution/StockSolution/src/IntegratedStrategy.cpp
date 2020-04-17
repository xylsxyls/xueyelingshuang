#include "IntegratedStrategy.h"
#include "StockStrategy/StockStrategyAPI.h"
#include <algorithm>
#include "StockFund/StockFundAPI.h"
#include "StockMarket/StockMarketAPI.h"
#include "StockSolution.h"
#include "AvgFundHighScore.h"
#include "ObserveStrategy.h"
#include "SolutionInfo.h"

IntegratedStrategy::IntegratedStrategy()
{
	m_solutionType = INTEGRATED_STRATEGY;
}

void IntegratedStrategy::init(const std::vector<ChooseParam>& vecChooseParam,
	std::map<SolutionType, std::shared_ptr<Solution>>* solutionMap)
{
	m_vecChooseParam = vecChooseParam;
	m_solutionMap = solutionMap;
}

void IntegratedStrategy::setSolutionInfo(const std::shared_ptr<SolutionInfo>& solutionInfo)
{
	m_solutionInfo = solutionInfo;
}

bool IntegratedStrategy::buy(std::vector<std::pair<std::string, StockInfo>>& buyStock, const IntDateTime& date)
{
	int32_t index = -1;
	while (index++ != m_vecChooseParam.size() - 1)
	{
		const ChooseParam& chooseParam = m_vecChooseParam[index];
		setEveryChooseParam(chooseParam);
		if (m_useSolution->buy(buyStock, date))
		{
			return true;
		}
	}
	return false;
}

bool IntegratedStrategy::sell(std::vector<std::pair<std::string, StockInfo>>& sellStock, const IntDateTime& date)
{
	sellStock.clear();
	std::vector<std::string> vecOwnedStock = m_solutionInfo->m_fund->ownedStock();

	bool result = false;
	int32_t index = -1;
	while (index++ != vecOwnedStock.size() - 1)
	{
		const std::string& stock = vecOwnedStock[index];
		std::shared_ptr<ChooseParam> spChooseParam = m_solutionInfo->m_fund->stockChooseParam(stock);
		setEveryChooseParam(*spChooseParam);
		std::vector<std::pair<std::string, StockInfo>> solutionSellStock;
		result = (result || m_useSolution->sell(solutionSellStock, date));
		int32_t index = -1;
		while (index++ != solutionSellStock.size() - 1)
		{
			sellStock.push_back(solutionSellStock[index]);
		}
	}
	return result;
}

void IntegratedStrategy::setEveryChooseParam(const ChooseParam& chooseParam)
{
	if (chooseParam.m_isObserve)
	{
		changeUseSolution(OBSERVE_STRATEGY);
		std::shared_ptr<ObserveStrategy> observeSolution = std::dynamic_pointer_cast<ObserveStrategy>(m_useSolution);
		std::shared_ptr<Solution> observeUseSolution = m_solutionMap->find(chooseParam.m_solutionType)->second;
		observeUseSolution->setStockFund(m_solutionInfo->m_fund);
		observeUseSolution->setFilterStock(m_solutionInfo->m_filterStock);
		observeUseSolution->setChooseParam(chooseParam);
		observeSolution->setSolution(observeUseSolution);
	}
	else
	{
		changeUseSolution(chooseParam.m_solutionType);
		m_useSolution->setStockFund(m_solutionInfo->m_fund);
		m_useSolution->setFilterStock(m_solutionInfo->m_filterStock);
		m_useSolution->setChooseParam(chooseParam);
	}
}

void IntegratedStrategy::changeUseSolution(SolutionType solutionType)
{
	auto itSolution = m_solutionMap->find(solutionType);
	if (itSolution == m_solutionMap->end())
	{
		m_useSolution = nullptr;
		return;
	}
	m_useSolution = itSolution->second;
}