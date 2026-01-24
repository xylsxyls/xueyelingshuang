#pragma once
#include "CompetitionManager.h"

class VerifyManager
{
private:
	VerifyManager();

public:
	static VerifyManager& instance();

public:
	std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>> verifyHistory(
		uint32_t beginTime, uint32_t endTime, uint32_t times,
		StrategyMode mode, const CompetitionConfig& originalConfig);

	std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>> verifyFuture(uint32_t beginTime, uint32_t endTime, uint32_t times,
		StrategyMode mode, const CompetitionConfig& originalConfig);

	std::pair<std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>,
		std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>> verifyHistoryFuture(uint32_t beginTime, uint32_t endTime, uint32_t historyTimes, uint32_t futureTimes,
		StrategyMode mode, const CompetitionConfig& originalConfig);

	void printDetectMap(const std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>& detectMap,
		bool showRank = false, bool showAvg = false);

	const std::pair<std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>,
		std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>>& getResult();

	std::vector<std::vector<std::pair<BigNumber, uint32_t>>> calcAvgMap(
		const std::map<uint32_t, std::vector<std::pair<BigNumber, uint32_t>>>& avgAnnualTReturnMap);

private:
	std::pair<std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>,
		std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>> m_result;
};