#include "EverydaySolutionTask.h"
#include "StockClient.h"
#include "StockDraw/StockDrawAPI.h"

EverydaySolutionTask::EverydaySolutionTask():
m_stockClient(nullptr)
{

}

void EverydaySolutionTask::DoTask()
{
	SolutionWidgetParam solutionWidgetParam;
	solutionWidgetParam.m_date = m_stockClient->m_today;
	solutionWidgetParam.m_buyStock = m_stockClient->m_buyStock;
	solutionWidgetParam.m_sellStock = m_stockClient->m_sellStock;
	StockDraw::instance().showSolution(solutionWidgetParam);
}

void EverydaySolutionTask::setParam(StockClient* stockClient)
{
	m_stockClient = stockClient;
}