#include "VerifyTask.h"
#include "CompetitionManager.h"
#include "VerifyManager.h"
#include "Quant.h"

VerifyTask::VerifyTask() :
m_showRank(false),
m_showAvg(false),
m_showChart(false)
{

}

void VerifyTask::DoTask()
{
	if (m_historyTimes != -1 && m_futureTimes == -1)
	{
		auto detectMap = VerifyManager::instance().verifyHistory(m_beginTime, m_endTime, m_historyTimes, m_mode, m_config);
		if (m_showRank || m_showAvg)
		{
			VerifyManager::instance().printDetectMap(detectMap, m_showRank, m_showAvg);
		}
	}
	else if (m_historyTimes == -1 && m_futureTimes != -1)
	{
		auto detectMap = VerifyManager::instance().verifyFuture(m_beginTime, m_endTime, m_futureTimes, m_mode, m_config);
		if (m_showRank || m_showAvg)
		{
			VerifyManager::instance().printDetectMap(detectMap, m_showRank, m_showAvg);
		}
	}
	else if (m_historyTimes != -1 && m_futureTimes != -1)
	{
		auto detectMap = VerifyManager::instance().verifyHistoryFuture(m_beginTime, m_endTime,
			m_historyTimes, m_futureTimes, m_mode, m_config);
		if (m_showRank || m_showAvg)
		{
			RCSend("开始打印历史验证");
			VerifyManager::instance().printDetectMap(detectMap.first, m_showRank, m_showAvg);
			RCSend("开始打印未来验证");
			VerifyManager::instance().printDetectMap(detectMap.second, m_showRank, m_showAvg);
		}
		emit m_client->historyFutureSignal();
	}
}

void VerifyTask::setParam(Quant* client, uint32_t beginTime, uint32_t endTime, uint32_t historyTimes, uint32_t futureTimes,
	StrategyMode mode, const CompetitionConfig& config, bool showRank, bool showAvg, bool showChart)
{
	m_client = client;
	m_beginTime = beginTime;
	m_endTime = endTime;
	m_historyTimes = historyTimes;
	m_futureTimes = futureTimes;
	m_mode = mode;
	m_config = config;
	m_showRank = showRank;
	m_showAvg = showAvg;
	m_showChart = showChart;
}