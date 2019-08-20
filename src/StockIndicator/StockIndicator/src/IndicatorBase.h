#pragma once
#include "BigNumber/BigNumberAPI.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include "StockIndicatorMacro.h"

struct StockIndicatorAPI IndicatorBase
{
	//日期
	IntDateTime m_date;
	//是否需要买
	bool m_needBuy;
	//是否需要卖
	bool m_needSell;
	//mairu比例
	BigNumber m_buyPercent;
	//maichu比例
	BigNumber m_sellPercent;
	//mairu分数
	BigNumber m_buyScore;
	//maichu分数
	BigNumber m_sellScore;

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	//gupiao代码
	std::string m_stock;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};