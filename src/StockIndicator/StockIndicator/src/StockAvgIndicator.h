#pragma once
#include "IndicatorManagerBase.h"
#include "StockIndicatorMacro.h"

struct StockAvg;

/** wr���zhibiao
*/
class StockIndicatorAPI StockAvgIndicator : public IndicatorManagerBase < StockAvg >
{
public:
	/** ��ԭʼ���ݼ��ص���������
	*/
	void load();

protected:
	/** ��������zhibiao
	*/
	void calc();
};