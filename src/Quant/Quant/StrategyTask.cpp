#include "StrategyTask.h"
#include <iostream>
#include <algorithm>
#include <cmath>

StrategyTask::StrategyTask(int32_t taskId,
	const std::shared_ptr<Strategy>& spStrategy,
	const std::shared_ptr<Market>& spMarket,
	uint32_t beginTime,
	uint32_t endTime,
	const std::vector<std::string>& stocks,
	int32_t initialFund,
	LockFreeQueue<StrategyResult>* pResultQueue,
	std::atomic<uint32_t>* pCompletedCount,
	uint32_t totalTasks,
	std::atomic<bool>* pStopFlag) :
	CTask(taskId),
	m_spStrategy(spStrategy),
	m_spMarket(spMarket),
	m_beginTime(beginTime),
	m_endTime(endTime),
	m_stocks(stocks),
	m_initialFund(initialFund),
	m_pResultQueue(pResultQueue),
	m_pCompletedCount(pCompletedCount),
	m_totalTasks(totalTasks),
	m_pStopFlag(pStopFlag),
	m_exit(false)
{

}

void StrategyTask::DoTask()
{
	if (m_exit || (m_pStopFlag && m_pStopFlag->load()))
	{
		return;
	}

	try
	{
		if (!m_spStrategy || !m_spMarket)
		{
			std::cerr << "Error: Strategy or Market is null" << std::endl;
			return;
		}

		// 初始化资金账户
		Fund fund;
		fund.init(m_initialFund);
		fund.setMarket(m_spMarket);

		// 设置策略的资金账户
		m_spStrategy->setFund(std::make_shared<Fund>(fund));

		// 运行策略回测
		StrategyResult result = runStrategy(fund);

		// 设置策略ID和参数
		result.strategyId = GetTaskId();
		result.params = m_spStrategy->getStrategyParams();

		// 将结果放入无锁队列
		if (m_pResultQueue && !m_exit)
		{
			m_pResultQueue->push(result);
		}

		// 更新完成计数（只在最后一次执行）
		if (m_pCompletedCount)
		{
			uint32_t completed = ++(*m_pCompletedCount);

			// 检查是否是最后一个任务
			if (completed == m_totalTasks && m_pResultQueue)
			{
				// 发送完成信号（通过一个特殊的结果）
				StrategyResult finalSignal;
				finalSignal.strategyId = -1;
				m_pResultQueue->push(finalSignal);
			}
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Exception in StrategyTask::DoTask: " << e.what() << std::endl;
	}
}

void StrategyTask::StopTask()
{
	m_exit = true;
}

StrategyResult StrategyTask::runStrategy(Fund& fund)
{
	uint32_t actualDays = 0;
	uint32_t totalDays = 0;

	// 获取所有交易日列表
	std::vector<uint32_t> tradingDays = getTradingDays();

	// 记录每日资产值用于计算指标
	std::vector<int32_t> dailyValues;
	dailyValues.reserve(tradingDays.size());

	int32_t peakValue = m_initialFund; // 峰值资产
	BigNumber maxDrawdown = 0; // 最大回撤
	BigNumber totalProfitArea = 0; // 总收益面积

	// 遍历所有交易日
	for (uint32_t i = 0; i < tradingDays.size(); ++i)
	{
		uint32_t date = tradingDays[i];

		if (m_exit || (m_pStopFlag && m_pStopFlag->load()))
		{
			break;
		}

		// 检查该日期是否有有效的市场数据
		bool hasValidData = false;
		for (const auto& stock : m_stocks)
		{
			const auto& dayData = m_spMarket->getStockData(stock, date);
			if (!dayData.empty() && dayData[0] > 0) // 第一个元素是日期，且不为0表示有数据
			{
				hasValidData = true;
				break;
			}
		}

		if (!hasValidData)
		{
			continue; // 跳过无效交易日（如停牌）
		}

		// 执行策略
		if (!m_spStrategy->onTradingDay(date))
		{
			std::cerr << "Strategy execution failed for date: " << date << std::endl;
		}

		// 记录当日资产
		int32_t currentValue = fund.getTotalValue(date, ObserveTime::TIME1410);
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

	// 如果是被中断的，计算实际交易天数
	if (m_exit || (m_pStopFlag && m_pStopFlag->load()))
	{
		// 重新计算实际交易天数
		actualDays = static_cast<uint32_t>(dailyValues.size());
	}

	// 强制平仓所有未完成交易
	if (!tradingDays.empty())
	{
		uint32_t lastDate = tradingDays.back();
		fund.closeAllTrades(lastDate, ObserveTime::TIME1410);
	}

	// 计算策略指标
	return calculateStrategyMetrics(fund, actualDays, totalDays, maxDrawdown, totalProfitArea, dailyValues);
}

StrategyResult StrategyTask::calculateStrategyMetrics(Fund& fund, uint32_t actualDays, uint32_t totalDays,
	BigNumber maxDrawdown, BigNumber totalProfitArea, const std::vector<int32_t>& dailyValues)
{
	StrategyResult result;

	if (actualDays == 0 || dailyValues.empty())
	{
		// 没有有效交易数据，返回默认值
		result.totalReturn = 0;
		result.annualReturn = 0;
		result.maxDrawdown = 0;
		result.winRate = 0;
		result.profitArea = 0;
		result.healthScore = 0;
		result.tradeDays = 0;
		result.totalDays = totalDays;
		return result;
	}

	// 计算总收益率
	int32_t finalValue = dailyValues.back();
	int32_t initialValue = m_initialFund;
	result.totalReturn = (BigNumber(finalValue - initialValue) / BigNumber(initialValue));

	// 计算年化收益率（按实际交易天数调整）
	if (actualDays > 0)
	{
		// 假设一年有250个交易日
		BigNumber years = BigNumber((int32_t)actualDays) / 250;
		if (years > 0)
		{
			result.annualReturn = ((BigNumber(1) + result.totalReturn).pow(BigNumber(1) / years)) - 1;
		}
		else
		{
			result.annualReturn = result.totalReturn;
		}
	}

	// 设置最大回撤
	result.maxDrawdown = maxDrawdown;

	// 计算胜率（基于每日收益）
	int32_t winningDays = 0;
	for (size_t i = 1; i < dailyValues.size(); ++i)
	{
		if (dailyValues[i] > dailyValues[i - 1])
		{
			winningDays++;
		}
	}

	if (dailyValues.size() > 1)
	{
		result.winRate = BigNumber(winningDays) / (int32_t)(dailyValues.size() - 1);
	}
	else
	{
		result.winRate = 0;
	}

	// 设置收益面积
	result.profitArea = totalProfitArea;

	// 计算健康值（基于收益稳定性和回撤）
	result.healthScore = calculateHealthScore(fund, result.totalReturn, maxDrawdown, result.winRate, dailyValues);

	result.tradeDays = actualDays;
	result.totalDays = totalDays;

	return result;
}

std::vector<uint32_t> StrategyTask::getTradingDays()
{
	std::vector<uint32_t> tradingDays;

	// 简单的实现：假设从开始日期到结束日期的每一天都是交易日
	// 实际应用中应该根据实际的交易日历来实现

	for (uint32_t date = m_beginTime; date <= m_endTime; ++date)
	{
		// 这里可以添加更复杂的逻辑来检查是否是交易日
		// 比如检查周末、节假日等

		// 简单的检查：只添加有效日期
		if (date >= m_beginTime && date <= m_endTime)
		{
			tradingDays.push_back(date);
		}
	}

	return tradingDays;
}

BigNumber StrategyTask::calculateHealthScore(Fund& fund, BigNumber totalReturn, BigNumber maxDrawdown,
	BigNumber winRate, const std::vector<int32_t>& dailyValues)
{
	BigNumber baseScore = 100;

	// 1. 基于总收益的评分（40%权重）
	BigNumber returnScore = 0;
	if (totalReturn > 0)
	{
		returnScore = (std::min)(BigNumber(40), totalReturn * 100); // 每1%收益得1分，最高40分
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