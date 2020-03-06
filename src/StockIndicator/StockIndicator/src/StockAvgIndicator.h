#pragma once
#include "IndicatorManagerBase.h"
#include "StockIndicatorMacro.h"

struct StockAvg;

/** avg���zhibiao
*/
class StockIndicatorAPI StockAvgIndicator : public IndicatorManagerBase
{
public:
	StockAvgIndicator();

public:
	/** ��ԭʼ���ݼ��ص���������
	*/
	void load();

	/** ��ȡָ������zhibiao
	@param [in] date ����
	@return ����ָ������zhibiao
	*/
	std::shared_ptr<StockAvg> day(const IntDateTime& date);

protected:
	/** ��������zhibiao
	*/
	void calc();
};