#pragma once
#include "StockDrawMacro.h"
#include "IntDateTime/IntDateTimeAPI.h"

class StockDrawAPI StockDraw
{
protected:
	/** 构造函数
	*/
	StockDraw();

public:
	/** 单一实例
	@return 返回单一实例
	*/
	static StockDraw& instance();

public:
	/** 显示avg的kxian图
	@param [in] stock gupiao代码
	@param [in] beginDate 开始时间
	@param [in] endDate 结束时间
	*/
	void showAvgKLine(const std::string& stock, const IntDateTime& beginDate, const IntDateTime& endDate);
};