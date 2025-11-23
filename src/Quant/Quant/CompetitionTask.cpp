#include "CompetitionTask.h"
#include "QuantStrategyManager.h"
#include "StrategyTask.h"
#include <algorithm>
#include "Util.h"
#include "Timer/TimerAPI.h"

CompetitionTask::CompetitionTask()
{

}

void CompetitionTask::DoTask()
{
	// 初始化线程池
	for (uint32_t i = 0; i < (uint32_t)g_config.m_cpuCoreCount * 2; ++i)
	{
		m_vecThreadId.push_back(CTaskThreadManager::Instance().Init());
	}

	std::vector<std::shared_ptr<StrategyTask>> vecStrategyTask;
	int32_t strategyCount = -1;
	for (auto itStrategy = m_competitionConfigMap.begin(); itStrategy != m_competitionConfigMap.end(); ++itStrategy)
	{
		StrategyMode currentStrategyMode = (StrategyMode)itStrategy->first;
		const CompetitionConfig& currentConfig = itStrategy->second;
		std::vector<std::vector<int32_t>> allVecParam = getAllParam(currentConfig.allParam);
		int32_t paramIndex = -1;
		while (paramIndex++ != allVecParam.size() - 1)
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
			spStrategy->setStrategyParam(allVecParam[paramIndex]);

			// 设置账户
			std::shared_ptr<Fund> spFund(new Fund);
			spStrategy->setFund(spFund);

			// 创建任务
			std::shared_ptr<StrategyTask> spStrategyTask(new StrategyTask);
			spStrategyTask->setParam(currentConfig.beginTime, currentConfig.endTime, currentConfig.stocks,
				spStrategy, currentConfig.marketData, currentConfig.initialFund, &m_resultQueue);
			vecStrategyTask.push_back(spStrategyTask);
			++strategyCount;
			
		}
	}
	++strategyCount;
	RCSend("正在多线程执行 %d 个小策略", strategyCount);

	std::atomic<int> lambda_count(0);
	Timer lambda_timer([&lambda_count, &strategyCount]()
	{
		if (g_config.m_completeTaskCount == 0)
		{
			RCSend("complete = 0");
			return;
		}
		++lambda_count;
		RCSend("complete = %u, remain = %.1lfs", g_config.m_completeTaskCount,
			(strategyCount - g_config.m_completeTaskCount) / (g_config.m_completeTaskCount / (double)lambda_count));
	}, 1000);
	lambda_timer.start();

	for (size_t index = 0; index < vecStrategyTask.size(); ++index)
	{
		uint32_t threadIndex = index % m_vecThreadId.size();
		CTaskThreadManager::Instance().GetThreadInterface(m_vecThreadId[threadIndex])->PostTask(vecStrategyTask[index]);
	}

	for (uint32_t i = 0; i < m_vecThreadId.size(); ++i)
	{
		CTaskThreadManager::Instance().GetThreadInterface(m_vecThreadId[i])->WaitForEnd();
	}

	StrategyResult result;
	while (m_resultQueue.pop(&result))
	{
		m_intermediateResults.push_back(result);
	}

	if (m_intermediateResults.empty())
	{
		RCSend("No results to rank");
		return;
	}

	// 按总收益率排序
	std::sort(m_intermediateResults.begin(), m_intermediateResults.end(),
		[](const StrategyResult& a, const StrategyResult& b) {
		return a.tReturn > b.tReturn;
	});

	RCSend("time = %.2lfmin\n", (int32_t)g_config.m_time.GetWatchTime() / 1000.0 / 60.0);
	for (size_t resultIndex = 0; (resultIndex < 10) && (resultIndex < m_intermediateResults.size()); ++resultIndex)
	{
		const StrategyResult& result = m_intermediateResults[resultIndex];
		std::string param;
		for (size_t index = 0; index < result.params.size(); ++index)
		{
			param += (index <= 1 ?
				(Util::observeTimeToWatchString((ObserveTime)result.params[index]) + ", ") :
				std::to_string(result.params[index]) + ", ");
		}
		if (!param.empty())
		{
			param.pop_back();
			param.pop_back();
		}

		RCSend("第%d名, param = %s, tProfit = %s元, trade = %s元, tAnnual = %s%%",
			(int32_t)(resultIndex + 1),
			param.c_str(),
			(m_intermediateResults[resultIndex].tReturn.toPrec(2).setDivParam(2) / 100.0).toString().c_str(),
			(m_intermediateResults[resultIndex].totalReturn.toPrec(2).setDivParam(2) / 100.0).toString().c_str(),
			(m_intermediateResults[resultIndex].annualTReturn.toPrec(16) * 100.0).toPrec(2).toString().c_str());
	}

	// 填充最终结果
	m_finalResult.rankedResults = m_intermediateResults;
	m_finalResult.totalStrategies = strategyCount;
	m_finalResult.completedStrategies = m_intermediateResults.size();

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

std::vector<std::vector<int32_t>> CompetitionTask::getAllParam(const std::vector<std::vector<int32_t>>& allParam)
{
	std::vector<std::vector<int32_t>> result;

	// 处理空输入的情况
	if (allParam.empty())
	{
		return result;
	}

	// 计算总组合数：各层元素数量的乘积
	size_t total = 1;
	for (size_t i = 0; i < allParam.size(); ++i)
	{
		// 如果任何一层为空，总组合数为0
		if (allParam[i].empty())
		{
			return result;
		}
		total *= allParam[i].size();
	}

	// 生成所有组合
	for (size_t i = 0; i < total; ++i)
	{
		std::vector<int32_t> combination;
		size_t remainder = i;

		// 为每层选择一个元素
		for (size_t j = 0; j < allParam.size(); ++j)
		{
			const std::vector<int32_t>& layer = allParam[j];
			size_t layerSize = layer.size();
			// 计算当前层的索引
			size_t index = remainder % layerSize;
			// 更新余数用于计算下一层
			remainder = remainder / layerSize;
			// 添加当前层选中的元素
			combination.push_back(layer[index]);
		}

		result.push_back(combination);
	}

	return result;
}
