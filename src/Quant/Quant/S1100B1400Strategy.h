#pragma once
#include "Strategy.h"
#include <vector>
#include <map>

/** S1100B1400策略
* 参数化日内交易策略，支持不同时间点和价格参数的组合
*/
class S1100B1400Strategy : public Strategy
{
public:
	/** 构造函数
	*/
	S1100B1400Strategy();

	/** 析构函数
	*/
	virtual ~S1100B1400Strategy() = default;

	/** 每个交易日的策略执行入口
	@param [in] date 当前交易日
	@return 返回是否执行成功
	*/
	bool onTradingDay(uint32_t date) override;

	/** 获取策略模式枚举
	@return 返回策略模式枚举值
	*/
	StrategyMode getStrategyMode() const override;

	/** 获取策略名称
	@return 返回策略名称字符串
	*/
	std::string getStrategyName() const override;

private:
	/** 获取指定时间点的最佳卖价（RANGE0）
	@param [in] stock 股票代码
	@param [in] date 日期
	@param [in] time 时间点
	@return 返回最佳卖价，如果找不到返回0
	*/
	int32_t getBestSellPrice(const std::string& stock, uint32_t date, ObserveTime time) const;

	/** 获取指定时间点的最佳买价（RANGE0）
	@param [in] stock 股票代码
	@param [in] date 日期
	@param [in] time 时间点
	@return 返回最佳买价，如果找不到返回0
	*/
	int32_t getBestBuyPrice(const std::string& stock, uint32_t date, ObserveTime time) const;

	/** 获取时间段内的最佳买价（RANGENEXT）
	@param [in] stock 股票代码
	@param [in] date 日期
	@param [in] startTime 开始时间点
	@param [in] endTime 结束时间点
	@return 返回最佳买价，如果找不到返回0
	*/
	int32_t getRangeNextBestBuyPrice(const std::string& stock, uint32_t date,
		ObserveTime startTime, ObserveTime endTime) const;

	/** 获取时间段内的最佳卖价（RANGENEXT）
	@param [in] stock 股票代码
	@param [in] date 日期
	@param [in] startTime 开始时间点
	@param [in] endTime 结束时间点
	@return 返回最佳卖价，如果找不到返回0
	*/
	int32_t getRangeNextBestSellPrice(const std::string& stock, uint32_t date,
		ObserveTime startTime, ObserveTime endTime) const;

	/** 检查是否应该反追买入
	@param [in] stock 股票代码
	@param [in] date 日期
	@param [in] sellPrice 卖出价格
	@param [in] chaseParam 反追参数
	@param [in] startTime 开始时间点
	@param [in] endTime 结束时间点
	@return 返回是否应该反追
	*/
	bool shouldChaseBuy(const std::string& stock, uint32_t date, int32_t sellPrice,
		int32_t chaseParam, ObserveTime startTime, ObserveTime endTime) const;

	/** 检查是否应该条件买入
	@param [in] stock 股票代码
	@param [in] date 日期
	@param [in] sellPrice 卖出价格
	@param [in] discountParam 降价参数
	@param [in] startTime 开始时间点
	@param [in] endTime 结束时间点
	@return 返回是否应该条件买入
	*/
	bool shouldConditionBuy(const std::string& stock, uint32_t date, int32_t sellPrice,
		int32_t discountParam, ObserveTime startTime, ObserveTime endTime) const;

	/** 获取下一个时间点
	@param [in] currentTime 当前时间点
	@return 返回下一个时间点，如果是最后一个时间点返回COUNT
	*/
	ObserveTime getNextTime(ObserveTime currentTime) const;

	/** 执行卖出操作
	@param [in] stock 股票代码
	@param [in] date 日期
	@param [in] sellTime 卖出时间点
	@return 返回卖出是否成功
	*/
	bool executeSell(const std::string& stock, uint32_t date, ObserveTime sellTime);

	/** 执行买入操作
	@param [in] stock 股票代码
	@param [in] date 日期
	@param [in] buyTime 买入时间点
	@param [in] price 买入价格
	@return 返回买入是否成功
	*/
	bool executeBuy(const std::string& stock, uint32_t date, ObserveTime buyTime, int32_t price);

	/** 获取ObserveTime在数据向量中的索引偏移量
	@param [in] time 时间点
	@return 返回索引偏移量
	*/
	size_t getObserveTimeOffset(ObserveTime time) const;

	/** 获取RangeTime在数据向量中的索引偏移量
	@param [in] range 时间范围
	@return 返回索引偏移量
	*/
	size_t getRangeTimeOffset(RangeTime range) const;

	/** 获取TransType在数据向量中的索引偏移量
	@param [in] transType 交易类型
	@return 返回索引偏移量
	*/
	size_t getTransTypeOffset(TransType transType) const;

private:
	// 策略状态记录
	struct TradeRecord
	{
		uint32_t date;
		std::string stock;
		int32_t sellPrice;
		int32_t buyPrice;
		ObserveTime sellTime;
		ObserveTime buyTime;
		bool chased; // 是否反追买入
		bool conditioned; // 是否条件买入
	};

	std::vector<TradeRecord> m_tradeRecords;
};