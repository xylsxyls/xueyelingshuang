#pragma once
#include "BigNumber/BigNumberAPI.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include "StockIndicatorMacro.h"

struct StockIndicatorAPI IndicatorBase
{
	//����
	IntDateTime m_date;
	//�Ƿ���Ҫ��
	bool m_needBuy;
	//�Ƿ���Ҫ��
	bool m_needSell;
	//mairu����
	BigNumber m_buyPercent;
	//maichu����
	BigNumber m_sellPercent;
	//mairu����
	BigNumber m_buyScore;
	//maichu����
	BigNumber m_sellScore;

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	//gupiao����
	std::string m_stock;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};