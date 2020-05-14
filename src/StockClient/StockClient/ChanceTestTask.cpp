#include "ChanceTestTask.h"
#include "StockRetest/StockRetestAPI.h"

ChanceTestTask::ChanceTestTask() :
m_solutionType(SOLUTION_INIT),
m_beginTime(0, 0),
m_endTime(0, 0),
m_maxDay(0),
m_stockClient(nullptr)
{

}

void ChanceTestTask::DoTask()
{
	if (m_beginTime > m_endTime)
	{
		return;
	}
	StockChanceRetest stockChanceRetest;
	stockChanceRetest.init(m_solutionType, m_vecChooseParam, m_beginTime, m_endTime, m_maxDay);
	stockChanceRetest.load();
	stockChanceRetest.run();
}

void ChanceTestTask::setParam(SolutionType solutionType,
	std::vector<ChooseParam>& vecChooseParam,
	const IntDateTime& beginTime,
	const IntDateTime& endTime,
	int32_t maxDay,
	StockClient* stockClient)
{
	m_solutionType = solutionType;
	m_vecChooseParam = vecChooseParam;
	m_beginTime = beginTime;
	m_endTime = endTime;
	m_maxDay = maxDay;
	m_stockClient = stockClient;
}