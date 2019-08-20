#pragma once
#include "IndicatorManagerBase.h"
#include "StockIndicatorMacro.h"

struct StockAvg;

/** avg相关zhibiao
*/
class StockIndicatorAPI StockAvgIndicator :
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	public IndicatorManagerBase < StockAvg >
{
#ifdef _MSC_VER
#pragma warning(pop)
#endif
public:
	/** 从原始数据加载到可用数据
	*/
	void load();

protected:
	/** 计算衍生zhibiao
	*/
	void calc();
};