#include "RealTestTask.h"
#include "StockRetest/StockRetestAPI.h"

RealTestTask::RealTestTask():
m_solutionType(SOLUTION_INIT),
m_beginTime(0, 0),
m_endTime(0, 0),
m_stockClient(nullptr)
{

}

void RealTestTask::DoTask()
{
	StockRealRetest stockRealRetest;
	stockRealRetest.init(m_solutionType, m_vecChooseParam, m_beginTime, m_endTime, std::vector<std::string>(), 200000, true);
	stockRealRetest.load();
	stockRealRetest.run();
}

void RealTestTask::setParam(SolutionType solutionType,
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