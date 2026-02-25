#include "StrategyTask.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include "Cini/CiniAPI.h"
#include "Util.h"
#include "StockManager.h"
#include "Semaphore/SemaphoreAPI.h"

StrategyTask::StrategyTask():
m_beginTime(0),
m_endTime(0),
m_spStrategy(nullptr),
m_spFund(nullptr),
m_resultQueue(nullptr),
m_resultSemaphore(nullptr),
m_forceEnd(false),
m_exit(false)
{

}

StrategyTask::~StrategyTask()
{

}

void StrategyTask::DoTask()
{
	if (!isParamValid())
	{
		RCSend("StrategyTask param is not valid");
		return;
	}

	m_spStrategy->setFund(m_spFund);

	uint32_t actualDays = 0;
	uint32_t totalDays = 0;

	// 获取所有交易日列表
	std::vector<int32_t> tradingDays = getTradingDays(StockManager::instance().getAllTradingDays());
	// 记录每日资产值用于计算指标
	std::vector<int32_t> dailyValues;
	dailyValues.reserve(tradingDays.size());

	int32_t peakValue = m_spFund->getInitialFund(); // 峰值资产
	BigNumber maxDrawdown = 0; // 最大回撤
	BigNumber totalProfitArea = 0; // 总收益面积

	bool isParamPass = true;
	uint32_t date = 0;
	// 遍历所有交易日
	for (uint32_t i = 0; i < tradingDays.size(); ++i)
	{
		if (m_exit)
		{
			break;
		}
		date = (uint32_t)tradingDays[i];
		// 执行策略
		if (!m_spStrategy->onTradingDay(date))
		{
			isParamPass = false;
			break;
		}

		// 记录当日资产
		int32_t currentValue = m_spFund->getTotalValue(date);
		dailyValues.push_back(currentValue);

		// 更新峰值和最大回撤
		if (currentValue > peakValue)
		{
			peakValue = currentValue;
		}

		BigNumber drawdown = (BigNumber(peakValue - currentValue) / BigNumber(peakValue));
		if (drawdown > maxDrawdown)
		{
			maxDrawdown = drawdown;
		}

		// 计算当日收益面积（简化版：当日收益率）
		if (i > 0)
		{
			int32_t prevValue = dailyValues[i - 1];
			BigNumber dailyReturn = (BigNumber(currentValue - prevValue) / BigNumber(prevValue));
			totalProfitArea = totalProfitArea + dailyReturn;
		}

		actualDays++;
		totalDays++;
	}

	// 中断退出
	if (m_exit)
	{
		return;
	}

	if (isParamPass)
	{
		// 强制平仓所有未完成交易
		m_spFund->closeAllTrades(date, !m_forceEnd);
		if (!tradingDays.empty() && m_forceEnd)
		{
			int32_t currentValue = m_spFund->getTotalValue((uint32_t)tradingDays.back());
			dailyValues.pop_back();
			dailyValues.push_back(currentValue);
		}

		// 计算策略指标
		StrategyResult result = calculateStrategyMetrics(actualDays, totalDays, maxDrawdown,
			totalProfitArea, dailyValues, date);

		// 设置策略ID和参数
		result.m_strategyMode = m_spStrategy->getStrategyMode();
		result.m_params = m_spStrategy->getStrategyParam();
		result.m_tradeLog = m_spStrategy->getFund()->exportTradeRecords();
		result.m_strategyLog = m_spStrategy->strategyLog();
		result.m_tradeCount = m_spStrategy->tradeCount();
		m_resultQueue->push(std::make_shared<StrategyResult>(result));
	}

	if (isParamPass)
	{
		++g_config.m_completeTaskCount;
		m_resultSemaphore->signal();
	}
	else
	{
		++g_config.m_ignoreTaskCount;
	}
}

void StrategyTask::StopTask()
{
	m_exit = true;
}

void StrategyTask::setParam(uint32_t beginTime, uint32_t endTime, const std::vector<std::string>& vecStock,
	const std::shared_ptr<Strategy>& spStrategy, const std::shared_ptr<Fund>& spFund, bool forceEnd,
	LockFreeQueue<std::shared_ptr<StrategyResult>>* resultQueue, Semaphore* resultSemaphore)
{
	m_beginTime = beginTime;
	m_endTime = endTime;
	m_vecStock = vecStock;
	m_spStrategy = spStrategy;
	m_spFund = spFund;
	m_forceEnd = forceEnd;
	m_resultQueue = resultQueue;
	m_resultSemaphore = resultSemaphore;
}

bool StrategyTask::isParamValid()
{
	if (m_beginTime < g_config.m_allBeginTime ||
		m_endTime < g_config.m_allBeginTime ||
		m_beginTime > m_endTime ||
		m_vecStock.empty() ||
		m_spStrategy == nullptr ||
		m_spFund == nullptr ||
		m_spFund->getMarket() == nullptr ||
		!m_spFund->getMarket()->hasStock(m_vecStock[0]) ||
		m_resultQueue == nullptr)
	{
		return false;
	}
	return true;
}

StrategyResult StrategyTask::calculateStrategyMetrics(uint32_t actualDays, uint32_t totalDays,
	BigNumber maxDrawdown, BigNumber totalProfitArea, const std::vector<int32_t>& dailyValues, uint32_t lastDate)
{
	StrategyResult result;

	if (actualDays <= 0 || dailyValues.empty())
	{
		// 没有有效交易数据，返回默认值
		result.m_totalReturn = 0;
		result.m_annualReturn = 0;
		result.m_maxDrawdown = 0;
		result.m_winRate = 0;
		result.m_profitArea = 0;
		result.m_healthScore = 0;
		result.m_tradeDays = 0;
		result.m_totalDays = totalDays;
		return result;
	}

	// 计算总收益 
	int32_t finalValue = dailyValues.back();
	int32_t initialValue = m_spFund->getInitialFund();
	result.m_totalReturn = finalValue - initialValue;

	// 做T总收益
	result.m_tReturn = m_spStrategy->getFund()->allTProfit(lastDate);

	// 计算年化收益率（按实际交易天数调整）
	// 假设一年有250个交易日
	BigNumber yearsProport = (BigNumber(250).toPrec(16) / BigNumber((int32_t)actualDays - 1));
	result.m_annualReturn = (BigNumber(1) + BigNumber(result.m_totalReturn).toPrec(16) / initialValue).pow(yearsProport) - 1;
	result.m_annualTReturn = (BigNumber(1) + BigNumber(result.m_tReturn).toPrec(16) / initialValue).pow(yearsProport) - 1;

	// 设置最大回撤
	result.m_maxDrawdown = maxDrawdown;

	// 计算胜率（基于每日收益）
	int32_t winningDays = 0;
	for (size_t i = 1; i < dailyValues.size(); ++i)
	{
		if (dailyValues[i] > dailyValues[i - 1])
		{
			winningDays++;
		}
	}
	result.m_winRate = BigNumber(winningDays) / (BigNumber((int32_t)dailyValues.size()) - 1).zero();

	// 设置收益面积
	result.m_profitArea = totalProfitArea;

	// 计算健康值（基于收益稳定性和回撤）
	result.m_healthScore = calculateHealthScore(result.m_totalReturn, maxDrawdown, result.m_winRate, dailyValues);

	result.m_tradeDays = actualDays;
	result.m_totalDays = totalDays;

	return result;
}

BigNumber StrategyTask::calculateHealthScore(int32_t totalReturn, BigNumber maxDrawdown,
	BigNumber winRate, const std::vector<int32_t>& dailyValues)
{
	BigNumber baseScore = 100;

	// 1. 基于总收益的评分（40%权重）
	BigNumber returnScore = 0;
	if (totalReturn > 0)
	{
		returnScore = (std::min)(40, totalReturn); // 每1%收益得1分，最高40分
	}

	// 2. 基于最大回撤的扣分（30%权重）
	BigNumber drawdownPenalty = 0;
	if (maxDrawdown > 0.05) // 超过5%开始扣分
	{
		drawdownPenalty = (std::min)(BigNumber(30), (maxDrawdown - 0.05) * 600); // 每0.1%回撤扣0.6分
	}

	// 3. 基于胜率的评分（20%权重）
	BigNumber winRateScore = 0;
	if (winRate > 0.5) // 胜率超过50%开始加分
	{
		winRateScore = (std::min)(BigNumber(20), (winRate - 0.5) * 40); // 每1%胜率得0.4分
	}

	// 4. 基于收益稳定性的评分（10%权重）
	BigNumber stabilityScore = 0;
	if (dailyValues.size() > 1)
	{
		// 计算收益波动率（简化版）
		BigNumber sumReturns = 0;
		BigNumber sumSquaredReturns = 0;

		for (size_t i = 1; i < dailyValues.size(); ++i)
		{
			BigNumber dailyReturn = (BigNumber(dailyValues[i] - dailyValues[i - 1]) / BigNumber(dailyValues[i - 1]));
			sumReturns = sumReturns + dailyReturn;
			sumSquaredReturns = sumSquaredReturns + (dailyReturn * dailyReturn);
		}

		BigNumber meanReturn = sumReturns / (int32_t)(dailyValues.size() - 1);
		BigNumber variance = (sumSquaredReturns / (int32_t)(dailyValues.size() - 1)) - (meanReturn * meanReturn);
		BigNumber volatility = variance.sqrt();

		// 波动率越低，稳定性得分越高
		if (volatility < 0.02) // 日波动率低于2%
		{
			stabilityScore = 10;
		}
		else if (volatility < 0.05) // 日波动率2%-5%
		{
			stabilityScore = 5;
		}
		// 超过5%不得分
	}

	// 计算最终健康值
	BigNumber healthScore = baseScore + returnScore - drawdownPenalty + winRateScore + stabilityScore;

	// 确保分数在合理范围内
	return (std::max)(BigNumber(0), (std::min)(BigNumber(100), healthScore));
}

std::vector<int32_t> StrategyTask::getTradingDays(const std::vector<int32_t>& allTradingDays)
{
	std::vector<int32_t> tradingDays;
	for (size_t index = 0; index < allTradingDays.size(); ++index)
	{
		uint32_t date = allTradingDays[index];
		if (date >= m_beginTime && date <= m_endTime)
		{
			tradingDays.push_back(date);
		}
	}
	return tradingDays;
}
