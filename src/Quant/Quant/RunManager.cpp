#include "RunManager.h"
#include "Config.h"
#include "CompetitionManager.h"
#include "Util.h"
#include "QuantStrategyManager.h"

RunManager::RunManager()
{

}

RunManager& RunManager::instance()
{
	static RunManager s_runManager;
	return s_runManager;
}

std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>> RunManager::runHistory(
	uint32_t beginTime, uint32_t endTime, uint32_t times,
	StrategyMode mode, const CompetitionConfig& originalConfig)
{
	RCSend("开始策略竞赛...");
	g_config.m_time.SetWatchTime(0);
	g_config.m_completeTaskCount = 0;

	CompetitionConfig config = originalConfig;

	int32_t profitBeginTime = beginTime;
	int32_t profitEndTime = endTime;
	std::string stock = "600975";

	// 创建市场数据
	auto marketData = std::make_shared<Market>();
	marketData->init(profitBeginTime, profitEndTime);
	marketData->addStock(stock);

	CompetitionManager::instance().addCompetition(mode, config);

	CompetitionManager::instance().setParam(true);

	// 开始竞赛
	if (!CompetitionManager::instance().startCompetition())
	{
		RCSend("策略竞赛启动失败");
		return std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>();
	}
	RCSend("策略竞赛已开始");
	std::shared_ptr<CompetitionTask> spCompetitionTask = CompetitionManager::instance().waitForEnd();
	std::map<int32_t, std::vector<std::shared_ptr<StrategyResult>>> detectResultMap = spCompetitionTask->getResultMap();
	std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>> detectMap;

	config.allParam.clear();
	std::map<StrategyMode, std::vector<std::vector<int32_t>>> paramMap;
	uint32_t rank = 0;
	for (auto it = detectResultMap.rbegin(); it != detectResultMap.rend() && rank < g_config.m_selectCount; ++it)
	{
		++rank;
		for (size_t index = 0; index < it->second.size(); ++index)
		{
			std::vector<std::shared_ptr<StrategyResult>> vecTimes;
			const std::shared_ptr<StrategyResult>& spStrategyResult = it->second[index];
			vecTimes.push_back(spStrategyResult);
			detectMap[it->first].push_back(vecTimes);
			paramMap[spStrategyResult->strategyMode].push_back(spStrategyResult->params);
		}
	}

	config.beginTime = beginTime;
	config.endTime = endTime;
	for (uint32_t timesIndex = 0; timesIndex < times; ++timesIndex)
	{
		int32_t historyBeginTime = Util::calcHistoryBeginTime(config.beginTime, config.endTime);
		config.endTime = config.beginTime;
		config.beginTime = historyBeginTime;
		for (auto itParam = paramMap.begin(); itParam != paramMap.end(); ++itParam)
		{
			config.allParam = itParam->second;
			CompetitionManager::instance().addCompetition(itParam->first, config);
		}
		CompetitionManager::instance().setParam(false);
		
		if (!CompetitionManager::instance().startCompetition())
		{
			RCSend("策略竞赛启动失败");
			continue;
		}
		RCSend("前%d名进行历史验证，第%u轮，开始时间：%u，结束时间：%u",
			rank, timesIndex + 1, config.beginTime, config.endTime);
		std::shared_ptr<CompetitionTask> spCompetitionTask = CompetitionManager::instance().waitForEnd();
		detectMap = Util::synthesisMap(detectMap, spCompetitionTask->getResultMap());
	}
	return detectMap;
}

std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>> RunManager::runFuture(
	uint32_t beginTime, uint32_t endTime, uint32_t times,
	StrategyMode mode, const CompetitionConfig& originalConfig)
{
	RCSend("开始策略竞赛...");
	g_config.m_time.SetWatchTime(0);
	g_config.m_completeTaskCount = 0;

	CompetitionConfig config = originalConfig;

	int32_t profitBeginTime = beginTime;
	int32_t profitEndTime = endTime;
	std::string stock = "600975";

	// 创建市场数据
	auto marketData = std::make_shared<Market>();
	marketData->init(profitBeginTime, profitEndTime);
	marketData->addStock(stock);

	CompetitionManager::instance().addCompetition(mode, config);

	CompetitionManager::instance().setParam(true);

	// 开始竞赛
	if (!CompetitionManager::instance().startCompetition())
	{
		RCSend("策略竞赛启动失败");
		return std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>();
	}
	RCSend("策略竞赛已开始");
	std::shared_ptr<CompetitionTask> spCompetitionTask = CompetitionManager::instance().waitForEnd();
	std::map<int32_t, std::vector<std::shared_ptr<StrategyResult>>> detectResultMap = spCompetitionTask->getResultMap();
	std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>> detectMap;

	config.allParam.clear();
	std::map<StrategyMode, std::vector<std::vector<int32_t>>> paramMap;
	uint32_t rank = 0;
	for (auto it = detectResultMap.rbegin(); it != detectResultMap.rend() && rank < g_config.m_selectCount; ++it)
	{
		++rank;
		for (size_t index = 0; index < it->second.size(); ++index)
		{
			std::vector<std::shared_ptr<StrategyResult>> vecTimes;
			const std::shared_ptr<StrategyResult>& spStrategyResult = it->second[index];
			vecTimes.push_back(spStrategyResult);
			detectMap[it->first].push_back(vecTimes);
			paramMap[spStrategyResult->strategyMode].push_back(spStrategyResult->params);
		}
	}

	config.beginTime = beginTime;
	config.endTime = endTime;
	for (uint32_t timesIndex = 0; timesIndex < times; ++timesIndex)
	{
		int32_t futureEndTime = Util::calcFutureEndTime(config.beginTime, config.endTime);
		config.beginTime = config.endTime;
		config.endTime = futureEndTime;
		for (auto itParam = paramMap.begin(); itParam != paramMap.end(); ++itParam)
		{
			config.allParam = itParam->second;
			CompetitionManager::instance().addCompetition(itParam->first, config);
		}
		CompetitionManager::instance().setParam(false);

		if (!CompetitionManager::instance().startCompetition())
		{
			RCSend("策略竞赛启动失败");
			continue;
		}
		RCSend("前%d名进行未来验证，第%u轮，开始时间：%u，结束时间：%u",
			rank, timesIndex + 1, config.beginTime, config.endTime);
		std::shared_ptr<CompetitionTask> spCompetitionTask = CompetitionManager::instance().waitForEnd();
		detectMap = Util::synthesisMap(detectMap, spCompetitionTask->getResultMap());
	}
	return detectMap;
}

std::pair<std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>,
	std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>> RunManager::runHistoryFuture(
	uint32_t beginTime, uint32_t endTime, uint32_t historyTimes, uint32_t futureTimes,
	StrategyMode mode, const CompetitionConfig& originalConfig)
{
	RCSend("开始策略竞赛...");
	g_config.m_time.SetWatchTime(0);
	g_config.m_completeTaskCount = 0;

	CompetitionConfig config = originalConfig;

	int32_t profitBeginTime = beginTime;
	int32_t profitEndTime = endTime;
	std::string stock = "600975";

	// 创建市场数据
	auto marketData = std::make_shared<Market>();
	marketData->init(profitBeginTime, profitEndTime);
	marketData->addStock(stock);

	CompetitionManager::instance().addCompetition(mode, config);

	CompetitionManager::instance().setParam(true);

	std::pair<std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>,
		std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>> result;

	// 开始竞赛
	if (!CompetitionManager::instance().startCompetition())
	{
		RCSend("策略竞赛启动失败");
		return result;
	}
	RCSend("策略竞赛已开始");
	std::shared_ptr<CompetitionTask> spCompetitionTask = CompetitionManager::instance().waitForEnd();
	std::map<int32_t, std::vector<std::shared_ptr<StrategyResult>>> detectResultMap = spCompetitionTask->getResultMap();

	config.allParam.clear();
	std::map<StrategyMode, std::vector<std::vector<int32_t>>> paramMap;
	uint32_t rank = 0;
	for (auto it = detectResultMap.rbegin(); it != detectResultMap.rend() && rank < g_config.m_selectCount; ++it)
	{
		++rank;
		for (size_t index = 0; index < it->second.size(); ++index)
		{
			std::vector<std::shared_ptr<StrategyResult>> vecTimes;
			const std::shared_ptr<StrategyResult>& spStrategyResult = it->second[index];
			vecTimes.push_back(spStrategyResult);
			result.first[it->first].push_back(vecTimes);
			result.second[it->first].push_back(vecTimes);
			paramMap[spStrategyResult->strategyMode].push_back(spStrategyResult->params);
		}
	}

	config.beginTime = beginTime;
	config.endTime = endTime;
	for (uint32_t timesIndex = 0; timesIndex < historyTimes; ++timesIndex)
	{
		int32_t historyBeginTime = Util::calcHistoryBeginTime(config.beginTime, config.endTime);
		config.endTime = config.beginTime;
		config.beginTime = historyBeginTime;
		for (auto itParam = paramMap.begin(); itParam != paramMap.end(); ++itParam)
		{
			config.allParam = itParam->second;
			CompetitionManager::instance().addCompetition(itParam->first, config);
		}
		CompetitionManager::instance().setParam(false);

		if (!CompetitionManager::instance().startCompetition())
		{
			RCSend("策略竞赛启动失败");
			continue;
		}
		RCSend("前%d名进行历史验证，第%u轮，开始时间：%u，结束时间：%u",
			rank, timesIndex + 1, config.beginTime, config.endTime);
		std::shared_ptr<CompetitionTask> spCompetitionTask = CompetitionManager::instance().waitForEnd();
		result.first = Util::synthesisMap(result.first, spCompetitionTask->getResultMap());
	}

	config.beginTime = beginTime;
	config.endTime = endTime;
	for (uint32_t timesIndex = 0; timesIndex < futureTimes; ++timesIndex)
	{
		int32_t futureEndTime = Util::calcFutureEndTime(config.beginTime, config.endTime);
		config.beginTime = config.endTime;
		config.endTime = futureEndTime;
		for (auto itParam = paramMap.begin(); itParam != paramMap.end(); ++itParam)
		{
			config.allParam = itParam->second;
			CompetitionManager::instance().addCompetition(itParam->first, config);
		}
		CompetitionManager::instance().setParam(false);

		if (!CompetitionManager::instance().startCompetition())
		{
			RCSend("策略竞赛启动失败");
			continue;
		}
		RCSend("前%d名进行未来验证，第%u轮，开始时间：%u，结束时间：%u",
			rank, timesIndex + 1, config.beginTime, config.endTime);
		std::shared_ptr<CompetitionTask> spCompetitionTask = CompetitionManager::instance().waitForEnd();
		result.second = Util::synthesisMap(result.second, spCompetitionTask->getResultMap());
	}

	m_result = result;
	return result;
}

void RunManager::printDetectMap(
	const std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>& detectMap)
{
	uint32_t rank = 0;
	for (auto itDetect = detectMap.rbegin(); itDetect != detectMap.rend(); ++itDetect)
	{
		++rank;
		const std::vector<std::vector<std::shared_ptr<StrategyResult>>>& detectTable = itDetect->second;
		size_t strategyIndex = -1;
		while (strategyIndex++ != detectTable.size() - 1)
		{
			const std::vector<std::shared_ptr<StrategyResult>>& vecTimes = detectTable[strategyIndex];
			size_t timeIndex = -1;
			while (timeIndex++ != vecTimes.size() - 1)
			{
				const std::shared_ptr<StrategyResult>& result = vecTimes[timeIndex];
				std::shared_ptr<Strategy> spStrategy = QuantStrategyManager::instance().createStrategy(result->strategyMode);
				std::string describe = spStrategy->describeParam(result->params);
				std::string modeName = spStrategy->getStrategyName();
				if (timeIndex == 0)
				{
					RCSend("第%u名, %s, tProfit = %s元, trade = %s元, tAnnual = %s%%, annual = %s%%",
						rank,
						modeName.c_str(),
						(BigNumber(result->tReturn).toPrec(2).setDivParam(2) / 100.0).toString().c_str(),
						(BigNumber(result->totalReturn).toPrec(2).setDivParam(2) / 100.0).toString().c_str(),
						(result->annualTReturn.toPrec(16) * 100.0).toPrec(2).toString().c_str(),
						(result->annualReturn.toPrec(16) * 100.0).toPrec(2).toString().c_str());
					RCSend("第%u名, param = %s", rank, describe.c_str());
				}
				else
				{
					RCSend("第%d次验证, tProfit = %s元, trade = %s元, tAnnual = %s%%, annual = %s%%",
						(int32_t)timeIndex,
						(BigNumber(result->tReturn).toPrec(2).setDivParam(2) / 100.0).toString().c_str(),
						(BigNumber(result->totalReturn).toPrec(2).setDivParam(2) / 100.0).toString().c_str(),
						(result->annualTReturn.toPrec(16) * 100.0).toPrec(2).toString().c_str(),
						(result->annualReturn.toPrec(16) * 100.0).toPrec(2).toString().c_str());
				}
			}
		}
	}
}

const std::pair<std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>,
	std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>>& RunManager::getResult()
{
	return m_result;
}
