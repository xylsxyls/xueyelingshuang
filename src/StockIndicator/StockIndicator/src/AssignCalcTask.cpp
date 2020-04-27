#include "AssignCalcTask.h"
#include "CalcDateWrTask.h"
#include "CalcDateRsiTask.h"
#include "CalcDateSarTask.h"
#include "CalcDateBollTask.h"

AssignCalcTask::AssignCalcTask():
m_threadId(0),
m_spMarket(nullptr),
m_date(0, 0),
m_wrIndicatorData(nullptr),
m_rsiIndicatorData(nullptr),
m_sarIndicatorData(nullptr),
m_bollIndicatorData(nullptr)
{

}

void AssignCalcTask::DoTask()
{
	m_spMarket->load();

	std::shared_ptr<CalcDateWrTask> spCalcDateWrTask(new CalcDateWrTask);
	spCalcDateWrTask->setParam(m_date, *m_spMarket, m_wrIndicatorData);
	CTaskThreadManager::Instance().GetThreadInterface(m_threadId)->PostTask(spCalcDateWrTask);

	std::shared_ptr<CalcDateRsiTask> spCalcDateRsiTask(new CalcDateRsiTask);
	spCalcDateRsiTask->setParam(m_date, *m_spMarket, m_rsiIndicatorData);
	CTaskThreadManager::Instance().GetThreadInterface(m_threadId)->PostTask(spCalcDateRsiTask);

	std::shared_ptr<CalcDateSarTask> spCalcDateSarTask(new CalcDateSarTask);
	spCalcDateSarTask->setParam(m_date, *m_spMarket, m_sarIndicatorData);
	CTaskThreadManager::Instance().GetThreadInterface(m_threadId)->PostTask(spCalcDateSarTask);

	std::shared_ptr<CalcDateBollTask> spCalcDateBollTask(new CalcDateBollTask);
	spCalcDateBollTask->setParam(m_date, *m_spMarket, m_bollIndicatorData);
	CTaskThreadManager::Instance().GetThreadInterface(m_threadId)->PostTask(spCalcDateBollTask);
}

void AssignCalcTask::setParam(uint32_t threadId,
	const std::shared_ptr<StockMarket>& spMarket,
	const IntDateTime& date,
	std::map<std::string, std::vector<std::vector<std::string>>>* wrIndicatorData,
	std::map<std::string, std::vector<std::vector<std::string>>>* rsiIndicatorData,
	std::map<std::string, std::vector<std::vector<std::string>>>* sarIndicatorData,
	std::map<std::string, std::vector<std::vector<std::string>>>* bollIndicatorData)
{
	m_threadId = threadId;
	m_spMarket = spMarket;
	m_date = date;
	m_wrIndicatorData = wrIndicatorData;
	m_rsiIndicatorData = rsiIndicatorData;
	m_sarIndicatorData = sarIndicatorData;
	m_bollIndicatorData = bollIndicatorData;
}