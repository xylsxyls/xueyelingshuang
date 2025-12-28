#include "CompetitionTask.h"
#include "QuantStrategyManager.h"
#include "StrategyTask.h"
#include <algorithm>
#include "Util.h"
#include "Timer/TimerAPI.h"
#include "StrategyResultTask.h"

CompetitionTask::CompetitionTask() :
m_isShowResult(false)
{

}

void CompetitionTask::DoTask()
{
	// 初始化线程池
	for (uint32_t i = 0; i < (uint32_t)g_config.m_cpuCoreCount * 2; ++i)
	{
		m_vecThreadId.push_back(CTaskThreadManager::Instance().Init());
	}

	m_resultThreadId = CTaskThreadManager::Instance().Init();

	std::vector<std::shared_ptr<StrategyTask>> vecStrategyTask;
	int32_t strategyCount = -1;
	for (auto itStrategy = m_competitionConfigMap.begin(); itStrategy != m_competitionConfigMap.end(); ++itStrategy)
	{
		StrategyMode currentStrategyMode = (StrategyMode)itStrategy->first;
		const CompetitionConfig& currentConfig = itStrategy->second;
		int32_t paramIndex = -1;
		while (paramIndex++ != currentConfig.allParam.size() - 1)
		{
			// 创建策略实例
			auto spStrategy = QuantStrategyManager::instance().createStrategy(currentStrategyMode);
			if (spStrategy == nullptr)
			{
				RCSend("Failed to create strategy for parameters");
				continue;
			}

			// 初始化策略
			spStrategy->init(currentConfig.beginTime, currentConfig.endTime);

			// 设置市场数据
			spStrategy->setMarket(currentConfig.marketData);

			// 添加股票
			for (const auto& stock : currentConfig.stocks)
			{
				spStrategy->addStock(stock);
			}

			// 设置策略参数
			spStrategy->setStrategyParam(currentConfig.allParam[paramIndex]);

			// 设置账户
			std::shared_ptr<Fund> spFund(new Fund);
			spStrategy->setFund(spFund);

			// 创建任务
			std::shared_ptr<StrategyTask> spStrategyTask(new StrategyTask);
			spStrategyTask->setParam(currentConfig.beginTime, currentConfig.endTime, currentConfig.stocks,
				spStrategy, currentConfig.marketData, currentConfig.initialFund, &m_resultQueue, &m_resultSemaphore);
			vecStrategyTask.push_back(spStrategyTask);
			++strategyCount;
		}
	}
	++strategyCount;
	RCSend("正在多线程执行 %d 个小策略", strategyCount);

	std::atomic<int> lambda_count(0);
	Timer lambda_timer([&lambda_count, &strategyCount]()
	{
		++lambda_count;
		if (g_config.m_completeTaskCount == 0)
		{
			RCSend("complete = 0");
			return;
		}
		RCSend("complete = %u, remain = %.1lfs", g_config.m_completeTaskCount + g_config.m_ignoreTaskCount,
			(strategyCount - g_config.m_ignoreTaskCount - g_config.m_completeTaskCount) /
			(g_config.m_completeTaskCount / (double)lambda_count));
	}, 1000);
	lambda_timer.start();

	for (size_t index = 0; index < vecStrategyTask.size(); ++index)
	{
		uint32_t threadIndex = index % (int32_t)m_vecThreadId.size();
		CTaskThreadManager::Instance().GetThreadInterface(m_vecThreadId[threadIndex])->PostTask(vecStrategyTask[index]);
	}

	std::atomic<bool> isComplete = false;
	std::shared_ptr<StrategyResultTask> spResultTask(new StrategyResultTask);
	spResultTask->setParam(&m_resultQueue, &m_resultSemaphore, &m_resultMap, &isComplete);
	CTaskThreadManager::Instance().GetThreadInterface(m_resultThreadId)->PostTask(spResultTask);

	for (uint32_t i = 0; i < m_vecThreadId.size(); ++i)
	{
		CTaskThreadManager::Instance().GetThreadInterface(m_vecThreadId[i])->WaitForEnd();
	}

	isComplete = true;
	m_resultSemaphore.signal();
	CTaskThreadManager::Instance().GetThreadInterface(m_resultThreadId)->WaitForEnd();

	RCSend("time = %.2lfmin, allTaskCount = %u\n", (int32_t)g_config.m_time.GetWatchTime() / 1000.0 / 60.0,
		(uint32_t)(g_config.m_completeTaskCount + g_config.m_ignoreTaskCount));

	return;

	// 填充最终结果
	std::vector<StrategyResult> m_intermediateResults;
	m_finalResult.rankedResults = m_intermediateResults;
	m_finalResult.totalStrategies = strategyCount;
	m_finalResult.completedStrategies = (uint32_t)m_intermediateResults.size();

	// 计算统计指标
	if (!m_intermediateResults.empty())
	{
		m_finalResult.bestReturn = m_intermediateResults.front().totalReturn;
		m_finalResult.worstReturn = m_intermediateResults.back().totalReturn;

		BigNumber sumReturn = 0;
		BigNumber sumAnnualReturn = 0;
		BigNumber sumMaxDrawdown = 0;
		BigNumber sumWinRate = 0;
		BigNumber sumProfitArea = 0;
		BigNumber sumHealthScore = 0;

		for (const auto& result : m_intermediateResults)
		{
			sumReturn = sumReturn + result.totalReturn;
			sumAnnualReturn = sumAnnualReturn + result.annualReturn;
			sumMaxDrawdown = sumMaxDrawdown + result.maxDrawdown;
			sumWinRate = sumWinRate + result.winRate;
			sumProfitArea = sumProfitArea + result.profitArea;
			sumHealthScore = sumHealthScore + result.healthScore;
		}

		BigNumber count = (int32_t)m_intermediateResults.size();
		m_finalResult.averageReturn = sumReturn / count;
		m_finalResult.averageAnnualReturn = sumAnnualReturn / count;
		m_finalResult.averageMaxDrawdown = sumMaxDrawdown / count;
		m_finalResult.averageWinRate = sumWinRate / count;
		m_finalResult.averageProfitArea = sumProfitArea / count;
		m_finalResult.averageHealthScore = sumHealthScore / count;

		// 计算中位数收益率
		size_t midIndex = m_intermediateResults.size() / 2;
		if (m_intermediateResults.size() % 2 == 0)
		{
			m_finalResult.medianReturn = (m_intermediateResults[midIndex - 1].totalReturn +
				m_intermediateResults[midIndex].totalReturn) / 2;
		}
		else
		{
			m_finalResult.medianReturn = m_intermediateResults[midIndex].totalReturn;
		}

		// 计算标准差
		BigNumber variance = 0;
		for (const auto& result : m_intermediateResults)
		{
			BigNumber diff = result.totalReturn - m_finalResult.averageReturn;
			variance = variance + (diff * diff);
		}
		variance = variance / count;
		m_finalResult.stdDevReturn = variance.sqrt();
	}

	RCSend("总策略数: % u\n"
		"完成策略数: % u\n"
		"最佳收益率: % s\n"
		"最差收益率: % s\n"
		"平均收益率: % s\n"
		"平均年化收益率: % s\n"
		"平均最大回撤: % s\n"
		"平均胜率: % s\n"
		"平均收益面积: % s\n"
		"平均健康值: % s\n"
		"中位数收益率: % s\n"
		"收益率标准差: % s",
		m_finalResult.totalStrategies,
		m_finalResult.completedStrategies,
		m_finalResult.bestReturn.toPrec(2).toString().c_str(),
		m_finalResult.worstReturn.toPrec(2).toString().c_str(),
		m_finalResult.averageReturn.toPrec(2).toString().c_str(),
		m_finalResult.averageAnnualReturn.toPrec(2).toString().c_str(),
		m_finalResult.averageMaxDrawdown.toPrec(2).toString().c_str(),
		m_finalResult.averageWinRate.toPrec(2).toString().c_str(),
		m_finalResult.averageProfitArea.toPrec(2).toString().c_str(),
		m_finalResult.averageHealthScore.toPrec(2).toString().c_str(),
		m_finalResult.medianReturn.toPrec(2).toString().c_str(),
		m_finalResult.stdDevReturn.toPrec(2).toString().c_str());

	RCSend("Competition completed. Total results processed");
}

void CompetitionTask::addParam(StrategyMode strategyMode, const CompetitionConfig& config)
{
	m_competitionConfigMap[(int32_t)strategyMode] = config;
}

const std::map<int32_t, std::vector<std::shared_ptr<StrategyResult>>>& CompetitionTask::getResultMap()
{
	return m_resultMap;
}

void CompetitionTask::printResultMap(uint32_t showCount)
{
	uint32_t rank = 0;
	for (auto it = m_resultMap.rbegin(); it != m_resultMap.rend() && (showCount == -1 ? true : (rank < showCount)); ++it)
	{
		++rank;
		for (size_t index = 0; index < it->second.size(); ++index)
		{
			const std::shared_ptr<StrategyResult>& result = it->second[index];
			std::shared_ptr<Strategy> spStrategy = QuantStrategyManager::instance().createStrategy(result->strategyMode);
			std::string describe = spStrategy->describeParam(result->params);
			std::string modeName = spStrategy->getStrategyName();
			RCSend("第%u名, %s, tProfit = %s元, trade = %s元, tAnnual = %s%%, annual = %s%%",
				rank,
				modeName.c_str(),
				(BigNumber(result->tReturn).toPrec(2).setDivParam(2) / 100.0).toString().c_str(),
				(BigNumber(result->totalReturn).toPrec(2).setDivParam(2) / 100.0).toString().c_str(),
				(result->annualTReturn.toPrec(16) * 100.0).toPrec(2).toString().c_str(),
				(result->annualReturn.toPrec(16) * 100.0).toPrec(2).toString().c_str());
			RCSend("第%u名, param = %s", rank, describe.c_str());
		}
	}
}

void CompetitionTask::setParam(bool isShowResult)
{
	m_isShowResult = isShowResult;
}
