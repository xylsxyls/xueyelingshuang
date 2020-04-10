#include "OnceTestTask.h"
#include "StockRetest/StockRetestAPI.h"

OnceTestTask::OnceTestTask() :
m_solutionType(SOLUTION_INIT),
m_beginTime(0, 0),
m_endTime(0, 0),
m_stockClient(nullptr)
{

}

void OnceTestTask::DoTask()
{
	StockOnceRetest stockOnceRetest;
	stockOnceRetest.init(m_solutionType, m_vecChooseParam, m_beginTime, m_endTime, "20000000000", true);
	stockOnceRetest.load();
	stockOnceRetest.run();
}

void OnceTestTask::setParam(SolutionType solutionType,
	const std::vector<ChooseParam>& vecChooseParam,
	const IntDateTime& beginTime,
	const IntDateTime& endTime,
	StockClient* stockClient)
{
	m_solutionType = solutionType;
	m_vecChooseParam = vecChooseParam;
	m_beginTime = beginTime;
	m_endTime = endTime;
	m_stockClient = stockClient;
}