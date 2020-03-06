#pragma once
#include "IndicatorManagerBase.h"
#include "StockIndicatorMacro.h"

struct StockBoll;

/** boll���zhibiao
*/
class StockIndicatorAPI StockBollIndicator : public IndicatorManagerBase
{
public:
	StockBollIndicator();

public:
	/** ��ԭʼ���ݼ��ص���������
	*/
	void load();

	/** ��ȡָ������zhibiao
	@param [in] date ����
	@return ����ָ������zhibiao
	*/
	std::shared_ptr<StockBoll> day(const IntDateTime& date);

protected:
	/** ��������zhibiao
	*/
	void calc();
};