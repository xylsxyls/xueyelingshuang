#include "EverydaySolutionTask.h"
#include "StockClient.h"
#include "StockDraw/StockDrawAPI.h"
#include "StockFund/StockFundAPI.h"
#include "ConfigManager/ConfigManagerAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "StockMarket/StockMarketAPI.h"

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
	solutionWidgetParam.m_stockFund = &m_stockClient->m_stockFund;
	StockDraw::instance().showSolution();
}

void EverydaySolutionTask::setParam(StockClient* stockClient)
{
	m_stockClient = stockClient;
}