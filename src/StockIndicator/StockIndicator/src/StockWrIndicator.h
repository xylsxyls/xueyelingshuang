#pragma once
#include "IndicatorManagerBase.h"
#include "StockIndicatorMacro.h"

struct StockWr;

/** wr相关zhibiao
*/
class StockIndicatorAPI StockWrIndicator : public IndicatorManagerBase < StockWr >
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