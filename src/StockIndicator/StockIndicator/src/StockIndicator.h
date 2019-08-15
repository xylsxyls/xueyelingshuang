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
	/** 从redis中加载原始数据
	@param [in] stock gupiao代码
	@param [in] beginTime 开始时间
	@param [in] endTime 结束时间
	*/
	void loadFromRedis(const std::string& stock,
		const IntDateTime& beginTime = IntDateTime(0, 0),
		const IntDateTime& endTime = IntDateTime(0, 0));

	/** 清空
	*/
	void clear();

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

protected:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::vector<std::vector<std::string>> m_redisIndicatorData;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};