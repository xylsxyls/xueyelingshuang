#include "StockDraw.h"
#include "StockDrawWidget.h"
#include "SolutionWidget.h"

StockDraw::StockDraw():
m_solutionWidget(nullptr)
{
	init();
}

StockDraw& StockDraw::instance()
{
	static StockDraw s_stockDraw;
	return s_stockDraw;
}

void StockDraw::init()
{
	m_solutionWidget = new SolutionWidget;
}

void StockDraw::showAvgKLine(const std::string& stock, const IntDateTime& beginDate, const IntDateTime& endDate)
{
	StockDrawWidget stockDrawWidget;
	stockDrawWidget.setMarketParam(stock, beginDate, endDate);
	stockDrawWidget.setAvgMarketParam(stock, beginDate, endDate);
	stockDrawWidget.exec();
}

void StockDraw::showSolution(const SolutionWidgetParam& solutionWidgetParam)
{
	emit m_solutionWidget->solutionSignal(solutionWidgetParam);
}