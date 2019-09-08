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

void StockDraw::showAvgKLine()
{
	StockDrawWidget stockDrawWidget;
	stockDrawWidget.setAvgParam();
	stockDrawWidget.exec();
}