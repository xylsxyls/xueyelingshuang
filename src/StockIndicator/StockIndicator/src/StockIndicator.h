#pragma once
#include "StockIndicatorMacro.h"
#include <memory>
#include "IntDateTime/IntDateTimeAPI.h"

class StockWrIndicator;
class StockRsiIndicator;
class StockIndicatorAPI StockIndicator
{
protected:
	/** 构造函数
	*/
	StockIndicator();

public:
	/** 单一实例
	@return 返回单一实例
	*/
	static StockIndicator& instance();

public:
	/** 获取wr相关接口
	@param [in] stock gupiao代码
	@param [in] beginTime 开始时间
	@param [in] endTime 结束时间
	*/
	std::shared_ptr<StockWrIndicator> wr(const std::string& stock,
		const IntDateTime& beginTime = IntDateTime(0, 0),
		const IntDateTime& endTime = IntDateTime(0, 0));

	/** 获取rsi相关接口
	@param [in] stock gupiao代码
	@param [in] beginTime 开始时间
	@param [in] endTime 结束时间
	*/
	std::shared_ptr<StockRsiIndicator> rsi(const std::string& stock,
		const IntDateTime& beginTime = IntDateTime(0, 0),
		const IntDateTime& endTime = IntDateTime(0, 0));
};