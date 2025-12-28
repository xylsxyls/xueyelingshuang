#include "RunTask.h"
#include "CompetitionManager.h"
#include "RunManager.h"
#include "Quant.h"

RunTask::RunTask()
{

}

void RunTask::DoTask()
{
	if (m_historyTimes != -1 && m_futureTimes == -1)
	{
		auto detectMap = RunManager::instance().runHistory(m_beginTime, m_endTime, m_historyTimes, m_mode, m_config);
		RunManager::instance().printDetectMap(detectMap);
	}
	else if (m_historyTimes == -1 && m_futureTimes != -1)
	{
		auto detectMap = RunManager::instance().runFuture(m_beginTime, m_endTime, m_futureTimes, m_mode, m_config);
		RunManager::instance().printDetectMap(detectMap);
	}
	else if (m_historyTimes != -1 && m_futureTimes != -1)
	{
		auto detectMap = RunManager::instance().runHistoryFuture(m_beginTime, m_endTime,
			m_historyTimes, m_futureTimes, m_mode, m_config);
		//RCSend("开始打印历史验证");
		//RunManager::instance().printDetectMap(detectMap.first);
		//RCSend("开始打印未来验证");
		//RunManager::instance().printDetectMap(detectMap.second);
		RCSend("rank size = %d", detectMap.first.size());
		emit m_client->historyFutureSignal();
	}
}

void RunTask::setParam(Quant* client, uint32_t beginTime, uint32_t endTime, uint32_t historyTimes, uint32_t futureTimes,
	StrategyMode mode, const CompetitionConfig& config)
{
	m_client = client;
	m_beginTime = beginTime;
	m_endTime = endTime;
	m_historyTimes = historyTimes;
	m_futureTimes = futureTimes;
	m_mode = mode;
	m_config = config;
}