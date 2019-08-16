#pragma once
#include "StockIndicatorMacro.h"
#include <memory>
#include <map>
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
	/** 从redis中加载原始数据，加载完后需要立刻通过具体zhibiao接口获取使用
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
	@return 返回wr相关接口
	*/
	std::shared_ptr<StockWrIndicator> wr();

	/** 获取rsi相关接口
	@return 返回rsi相关接口
	*/
	std::shared_ptr<StockRsiIndicator> rsi();

protected:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::shared_ptr<std::vector<std::vector<std::string>>> m_redisIndicatorData;
	std::string m_loadStock;
	std::map<std::string, std::vector<int32_t>> m_indicatorIndex;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};