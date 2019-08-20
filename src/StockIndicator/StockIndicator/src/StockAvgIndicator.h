#pragma once
#include "IndicatorManagerBase.h"
#include "StockIndicatorMacro.h"

struct StockAvg;

/** wr相关zhibiao
*/
class StockIndicatorAPI StockAvgIndicator : public IndicatorManagerBase < StockAvg >
{
public:
	/** 从原始数据加载到可用数据
	*/
	void load();

protected:
	/** 计算衍生zhibiao
	*/
	void calc();
};