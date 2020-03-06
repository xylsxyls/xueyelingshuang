#pragma once
#include "IndicatorManagerBase.h"
#include "StockIndicatorMacro.h"

struct StockWr;

/** wr���zhibiao
*/
class StockIndicatorAPI StockWrIndicator : public IndicatorManagerBase
{
public:
	StockWrIndicator();

public:
	/** ��ԭʼ���ݼ��ص���������
	*/
	void load();

	/** ��ȡָ������zhibiao
	@param [in] date ����
	@return ����ָ������zhibiao
	*/
	std::shared_ptr<StockWr> day(const IntDateTime& date);

protected:
	/** ��������zhibiao
	*/
	void calc();
};