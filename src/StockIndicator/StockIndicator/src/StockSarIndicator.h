#pragma once
#include "IndicatorManagerBase.h"
#include "StockIndicatorMacro.h"

struct StockSar;

/** sar相关zhibiao
*/
class StockIndicatorAPI StockSarIndicator : public IndicatorManagerBase
{
public:
	StockSarIndicator();

public:
	/** 从原始数据加载到可用数据
	*/
	void load();

	/** 获取指定日期zhibiao
	@param [in] date 日期
	@return 返回指定日期zhibiao
	*/
	std::shared_ptr<StockSar> day(const IntDateTime& date);

protected:
	/** 计算衍生zhibiao
	*/
	void calc();
};