#pragma once
#include "IndicatorManagerBase.h"
#include "StockIndicatorMacro.h"

struct StockSar;

/** sar���zhibiao
*/
class StockIndicatorAPI StockSarIndicator : public IndicatorManagerBase
{
public:
	StockSarIndicator();

public:
	/** ��ԭʼ���ݼ��ص���������
	*/
	void load();

	/** ��ȡָ������zhibiao
	@param [in] date ����
	@return ����ָ������zhibiao
	*/
	std::shared_ptr<StockSar> day(const IntDateTime& date);

protected:
	/** ��������zhibiao
	*/
	void calc();
};