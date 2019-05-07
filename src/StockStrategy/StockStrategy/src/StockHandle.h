#pragma once
#include "BigNumber/BigNumberAPI.h"
#include "StockStrategyMacro.h"

struct StockStrategyAPI StockHandle
{
	//买或者卖
	bool m_isBuy;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	//股票代码
	std::string m_stock;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	//买则冻结，卖则解冻
	BigNumber m_freezeOrFree;
	//买卖价格
	BigNumber m_price;
	//买则冻结比例，卖则解冻比例，先买再冻结，先卖再解冻
	BigNumber m_rate;
	//购买之前解冻比例
	BigNumber m_freeBeforeBuy;

	/** 构造函数
	*/
	StockHandle()
	{
		m_isBuy = false;
		m_freezeOrFree = 0;
		m_price = 0;
		m_rate = 0;
		m_freeBeforeBuy = 0;
	}

	/** 构造函数
	*/
	StockHandle(bool isBuy, const std::string& stock, const BigNumber& freezeOrFree, const BigNumber& price, const BigNumber& rate, const BigNumber& freeBeforeBuy)
	{
		m_isBuy = isBuy;
		m_stock = stock;
		m_freezeOrFree = freezeOrFree;
		m_price = price;
		m_rate = rate;
		m_freeBeforeBuy = freeBeforeBuy;
	}
};