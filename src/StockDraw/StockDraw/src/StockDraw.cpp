#include "StockDraw.h"
#include "StockDrawWidget.h"

StockDraw::StockDraw()
{

}

StockDraw& StockDraw::instance()
{
	static StockDraw s_stockDraw;
	return s_stockDraw;
}

void StockDraw::showAvgKLine(const std::string& stock, const IntDateTime& beginDate, const IntDateTime& endDate)
{
	StockDrawWidget stockDrawWidget;
	stockDrawWidget.setAvgParam(stock, beginDate, endDate);
	stockDrawWidget.exec();
}