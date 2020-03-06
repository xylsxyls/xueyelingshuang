#pragma once
#include "IndicatorManagerBase.h"
#include "StockIndicatorMacro.h"

struct StockRsi;

/** rsi���zhibiao
*/
class StockIndicatorAPI StockRsiIndicator : public IndicatorManagerBase
{
public:
	StockRsiIndicator();

public:
	/** ��ԭʼ���ݼ��ص���������
	*/
	void load();

	/** ��ȡָ������zhibiao
	@param [in] date ����
	@return ����ָ������zhibiao
	*/
	std::shared_ptr<StockRsi> day(const IntDateTime& date);

protected:
	/** ��������zhibiao
	*/
	void calc();
};