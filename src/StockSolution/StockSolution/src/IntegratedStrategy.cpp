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

void IntegratedStrategy::init(const std::vector<ChooseParam>& vecChooseParam, StockStorageBase* storage)
{
	m_vecChooseParam = vecChooseParam;
	m_storage = storage;
}

bool IntegratedStrategy::buy(std::vector<std::pair<std::string, StockInfo>>& buyStock, const IntDateTime& date)
{
	buyStock.clear();
	int32_t index = -1;
	while (index++ != m_vecChooseParam.size() - 1)
	{
		const ChooseParam& chooseParam = m_vecChooseParam[index];
		setEveryChooseParam(chooseParam, date);
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

	std::set<ChooseParam> chooseParamSet;

	bool result = false;
	int32_t index = -1;
	while (index++ != vecOwnedStock.size() - 1)
	{
		const std::string& stock = vecOwnedStock[index];
		std::shared_ptr<ChooseParam> spChooseParam = m_solutionInfo->m_fund->stockChooseParam(stock);
		chooseParamSet.insert(*spChooseParam);
	}

	for (auto itChooseParam = chooseParamSet.begin(); itChooseParam != chooseParamSet.end(); ++itChooseParam)
	{
		setEveryChooseParam(*itChooseParam, date);
		std::vector<std::pair<std::string, StockInfo>> solutionSellStock;
		result = (m_useSolution->sell(solutionSellStock, date) || result);
		int32_t index = -1;
		while (index++ != solutionSellStock.size() - 1)
		{
			sellStock.push_back(solutionSellStock[index]);
			RCSend("sellstock = %s, useType = %d, useCountType = %d, isObserve = %d, solutionType = %d",
				solutionSellStock[index].first.c_str(),
				itChooseParam->m_useType,
				itChooseParam->m_useCountType,
				(int32_t)itChooseParam->m_isObserve,
				itChooseParam->m_solutionType);
		}
	}
	
	return result;
}

void IntegratedStrategy::setEveryChooseParam(const ChooseParam& chooseParam, const IntDateTime& date)
{
	if (chooseParam.m_isObserve)
	{
		m_useSolution = m_storage->solution(OBSERVE_STRATEGY);
		std::shared_ptr<ObserveStrategy> observeSolution = std::dynamic_pointer_cast<ObserveStrategy>(m_useSolution);
		std::shared_ptr<Solution> observeUseSolution = m_storage->solution(chooseParam.m_solutionType);
		
		observeUseSolution->setStockFund(m_solutionInfo->m_fund);

		IntDateTime moveDate = m_storage->moveDay(date, observeSolution->calcDays());
		observeUseSolution->setFilterStock(m_storage->filterStock(chooseParam.m_useType, moveDate));

		observeUseSolution->setChooseParam(chooseParam);
		observeSolution->setSolution(observeUseSolution);
	}
	else
	{
		m_useSolution = m_storage->solution(chooseParam.m_solutionType);
		m_useSolution->setStockFund(m_solutionInfo->m_fund);
		m_useSolution->setFilterStock(m_storage->filterStock(chooseParam.m_useType, date));
		m_useSolution->setChooseParam(chooseParam);
	}
	m_solutionInfo->m_chooseParam = chooseParam;
}