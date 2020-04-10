#pragma once
#include "BigNumber/BigNumberAPI.h"

struct StockInfo
{
	//价格
	BigNumber m_price;
	//百分比0 - 100
	BigNumber m_percent;
	//分数
	BigNumber m_score;
};