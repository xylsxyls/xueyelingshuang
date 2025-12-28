#pragma once
#include "CompetitionManager.h"

class RunManager
{
private:
	RunManager();

public:
	static RunManager& instance();

public:
	std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>> runHistory(
		uint32_t beginTime, uint32_t endTime, uint32_t times,
		StrategyMode mode, const CompetitionConfig& originalConfig);

	std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>> runFuture(uint32_t beginTime, uint32_t endTime, uint32_t times,
		StrategyMode mode, const CompetitionConfig& originalConfig);

	std::pair<std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>,
		std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>> runHistoryFuture(uint32_t beginTime, uint32_t endTime, uint32_t historyTimes, uint32_t futureTimes,
		StrategyMode mode, const CompetitionConfig& originalConfig);

	void printDetectMap(const std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>& detectMap);

	const std::pair<std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>,
		std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>>& getResult();

private:
	std::pair<std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>,
		std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>> m_result;
};