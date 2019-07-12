#pragma once
#include "BigNumber/BigNumberAPI.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include "StockIndicatorMacro.h"

class StockIndicatorAPI IndicatorBase
{
	friend class StockWrIndicator;
	friend class StockRsiIndicator;
public:
	/** 获取gupiao代码
	@return 返回gupiao代码
	*/
	std::string stock();

	/** 获取日期
	@return 返回日期
	*/
	IntDateTime date();

	/** 是否需要买
	@return 返回是否需要买
	*/
	bool needBuy();

	/** 是否需要卖
	@return 返回是否需要卖
	*/
	bool needSell();

	/** 买入比例
	*/
	BigNumber buyPercent();

	/** 卖出比例
	*/
	BigNumber sellPercent();

	/** 买入分数
	@return 返回买入分数
	*/
	BigNumber buyScore();

	/** 卖出分数
	@return 返回卖出分数
	*/
	BigNumber sellScore();

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::string m_stock;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	IntDateTime m_date;
	bool m_needBuy;
	bool m_needSell;
	BigNumber m_buyPercent;
	BigNumber m_sellPercent;
	BigNumber m_buyScore;
	BigNumber m_sellScore;
};