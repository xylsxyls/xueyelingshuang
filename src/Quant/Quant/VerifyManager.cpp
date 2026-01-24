#include "VerifyManager.h"
#include "Config.h"
#include "CompetitionManager.h"
#include "Util.h"
#include "QuantStrategyManager.h"
#include <algorithm>

VerifyManager::VerifyManager()
{

}

VerifyManager& VerifyManager::instance()
{
	static VerifyManager s_verifyManager;
	return s_verifyManager;
}

std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>> VerifyManager::verifyHistory(
	uint32_t beginTime, uint32_t endTime, uint32_t times,
	StrategyMode mode, const CompetitionConfig& originalConfig)
{
	RCSend("开始策略竞赛...");
	g_config.m_completeTaskCount = 0;
	g_config.m_ignoreTaskCount = 0;

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
		RCSend("第%u轮历史验证结束，正在合并map...", timesIndex + 1);
		detectMap = Util::synthesisMap(detectMap, spCompetitionTask->getResultMap());
		RCSend("第%u轮历史验证结束，合并map完成", timesIndex + 1);
	}
	m_result.first = detectMap;
	m_result.second.clear();
	return detectMap;
}

std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>> VerifyManager::verifyFuture(
	uint32_t beginTime, uint32_t endTime, uint32_t times,
	StrategyMode mode, const CompetitionConfig& originalConfig)
{
	RCSend("开始策略竞赛...");
	g_config.m_completeTaskCount = 0;
	g_config.m_ignoreTaskCount = 0;

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
		RCSend("第%u轮未来验证结束，正在合并map...", timesIndex + 1);
		detectMap = Util::synthesisMap(detectMap, spCompetitionTask->getResultMap());
		RCSend("第%u轮未来验证结束，合并map完成", timesIndex + 1);
	}
	m_result.first.clear();
	m_result.second = detectMap;
	return detectMap;
}

std::pair<std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>,
	std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>> VerifyManager::verifyHistoryFuture(
	uint32_t beginTime, uint32_t endTime, uint32_t historyTimes, uint32_t futureTimes,
	StrategyMode mode, const CompetitionConfig& originalConfig)
{
	RCSend("开始策略竞赛...");
	g_config.m_completeTaskCount = 0;
	g_config.m_ignoreTaskCount = 0;

	CompetitionConfig config = originalConfig;

	int32_t profitBeginTime = beginTime;
	int32_t profitEndTime = endTime;
	std::string stock = "600975";

	// 创建市场数据
	auto marketData = std::make_shared<Market>();
	marketData->init(profitBeginTime, profitEndTime);
	marketData->addStock(stock);

	CompetitionManager::instance().addCompetition(mode, config);

	CompetitionManager::instance().setParam(false);

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
		g_config.m_completeTaskCount = 0;
		g_config.m_ignoreTaskCount = 0;

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
		RCSend("第%u轮历史验证结束，正在合并map...", timesIndex + 1);
		result.first = Util::synthesisMap(result.first, spCompetitionTask->getResultMap());
		RCSend("第%u轮历史验证结束，合并map完成", timesIndex + 1);
	}

	config.beginTime = beginTime;
	config.endTime = endTime;
	for (uint32_t timesIndex = 0; timesIndex < futureTimes; ++timesIndex)
	{
		g_config.m_completeTaskCount = 0;
		g_config.m_ignoreTaskCount = 0;

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
		RCSend("第%u轮未来验证结束，正在合并map...", timesIndex + 1);
		result.second = Util::synthesisMap(result.second, spCompetitionTask->getResultMap());
		RCSend("第%u轮未来验证结束，合并map完成", timesIndex + 1);
	}
	m_result = result;
	return result;
}

void VerifyManager::printDetectMap(
	const std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>& detectMap,
	bool showRank, bool showAvg)
{
	std::map<uint32_t, std::vector<std::pair<BigNumber, uint32_t>>> avgAnnualTReturnMap;
	std::map<uint32_t, std::vector<std::pair<BigNumber, uint32_t>>> avgAnnualReturnMap;
	uint32_t timeSize = 0;
	uint32_t rank = 0;
	for (auto itDetect = detectMap.rbegin(); itDetect != detectMap.rend(); ++itDetect)
	{
		++rank;
		const std::vector<std::vector<std::shared_ptr<StrategyResult>>>& detectTable = itDetect->second;
		std::vector<std::pair<BigNumber, uint32_t>>& vecAvgAnnualTReturn = avgAnnualTReturnMap[rank];
		std::vector<std::pair<BigNumber, uint32_t>>& vecAvgAnnualReturn = avgAnnualReturnMap[rank];
		size_t strategyIndex = -1;
		while (strategyIndex++ != detectTable.size() - 1)
		{
			const std::vector<std::shared_ptr<StrategyResult>>& vecTimes = detectTable[strategyIndex];
			if (strategyIndex == 0)
			{
				vecAvgAnnualTReturn.resize(vecTimes.size() - 1);
				vecAvgAnnualReturn.resize(vecTimes.size() - 1);
				timeSize = (uint32_t)vecTimes.size();
			}
			
			size_t timeIndex = -1;
			while (timeIndex++ != vecTimes.size() - 1)
			{
				const std::shared_ptr<StrategyResult>& result = vecTimes[timeIndex];
				std::shared_ptr<Strategy> spStrategy = QuantStrategyManager::instance().createStrategy(result->strategyMode);
				std::string describe = spStrategy->describeParam(result->params);
				std::string modeName = spStrategy->getStrategyName();
				if (timeIndex == 0)
				{
					if (showRank)
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
				}
				else
				{
					vecAvgAnnualTReturn[timeIndex - 1].first =
						vecAvgAnnualTReturn[timeIndex - 1].first + result->annualTReturn;
					++(vecAvgAnnualTReturn[timeIndex - 1].second);
					vecAvgAnnualReturn[timeIndex - 1].first =
						vecAvgAnnualReturn[timeIndex - 1].first + result->annualReturn;
					++(vecAvgAnnualReturn[timeIndex - 1].second);
					if (showRank)
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

	if (showAvg)
	{
		std::vector<std::vector<std::pair<BigNumber, uint32_t>>> vecAllTReturn = calcAvgMap(avgAnnualTReturnMap);
		std::vector<std::vector<std::pair<BigNumber, uint32_t>>> vecAllReturn = calcAvgMap(avgAnnualReturnMap);

		uint32_t avgSize = (uint32_t)avgAnnualReturnMap.size() / g_config.m_avgCount;
		for (size_t timeIndex = 0; timeIndex < timeSize - 1; ++timeIndex)
		{
			for (size_t rankIndex = 0; rankIndex < vecAllReturn.size(); ++rankIndex)
			{
				std::vector<std::pair<BigNumber, uint32_t>>& vecRankAvgT = vecAllTReturn[rankIndex];
				std::vector<std::pair<BigNumber, uint32_t>>& vecRankAvg = vecAllReturn[rankIndex];
				BigNumber avgTReturn = vecRankAvgT[timeIndex].first.setDivParam() / (int)vecRankAvgT[timeIndex].second;
				BigNumber avgReturn = vecRankAvg[timeIndex].first.setDivParam() / (int)vecRankAvg[timeIndex].second;
				RCSend("第%d名-第%d名，avgTAnnual = %s%%, avgAnnual = %s%%",
					(int32_t)(rankIndex * avgSize + 1),
					(std::min)((int32_t)((rankIndex + 1) * avgSize), (int32_t)avgAnnualTReturnMap.size()),
					(avgTReturn.toPrec(16) * 100.0).toPrec(2).toString().c_str(),
					(avgReturn.toPrec(16) * 100.0).toPrec(2).toString().c_str());
			}
		}
	}
}

const std::pair<std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>,
	std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>>& VerifyManager::getResult()
{
	return m_result;
}

std::vector<std::vector<std::pair<BigNumber, uint32_t>>> VerifyManager::calcAvgMap(
	const std::map<uint32_t, std::vector<std::pair<BigNumber, uint32_t>>>& avgAnnualReturnMap)
{
	std::vector<std::vector<std::pair<BigNumber, uint32_t>>> vecAllReturn;
	std::vector<std::pair<BigNumber, uint32_t>> vecTReturn;
	uint32_t timeSize = 0;
	uint32_t avgSize = (uint32_t)avgAnnualReturnMap.size() / g_config.m_avgCount;
	uint32_t currentSize = 0;
	for (auto it = avgAnnualReturnMap.begin(); it != avgAnnualReturnMap.end(); ++it)
	{
		++currentSize;
		uint32_t currentIndex = currentSize / avgSize;
		const std::vector<std::pair<BigNumber, uint32_t>>& vecAvgAnnualTReturn = it->second;
		if (currentSize == 1)
		{
			vecTReturn.resize(vecAvgAnnualTReturn.size());
			timeSize = (uint32_t)vecAvgAnnualTReturn.size();
		}

		for (size_t timeIndex = 0; timeIndex < vecAvgAnnualTReturn.size(); ++timeIndex)
		{
			vecTReturn[timeIndex].first = vecTReturn[timeIndex].first + vecAvgAnnualTReturn[timeIndex].first;
			vecTReturn[timeIndex].second = vecTReturn[timeIndex].second + vecAvgAnnualTReturn[timeIndex].second;
		}
		if (currentSize == avgSize)
		{
			vecAllReturn.push_back(vecTReturn);
			vecTReturn.clear();
			currentSize = 0;
		}
	}
	if (!vecTReturn.empty())
	{
		vecAllReturn.push_back(vecTReturn);
		vecTReturn.clear();
		currentSize = 0;
	}

	return vecAllReturn;
}