#pragma once
#include "Strategy.h"
#include <vector>
#include <map>

/** Wave策略
@param 参数化日内交易策略，支持不同时间点和价格参数的组合
*/
class WaveStrategy : public Strategy
{
public:
	/** 构造函数
	*/
	WaveStrategy();

	/** 析构函数
	*/
	virtual ~WaveStrategy();

	/** 每个交易日的策略执行入口，从开始到结束依次循环
	@param [in] date 当前交易日
	@return 返回是否执行成功
	*/
	bool onTradingDay(uint32_t date);

	/** 描述参数
	@param [in] params 参数组
	@return 返回参数含义
	*/
	virtual std::string describeParam(const std::vector<int32_t>& params);

	/** 获取策略参数是否有效
	@return 返回策略参数是否有效
	*/
	virtual bool isStrategyParamValid() const;

private:
	/** 根据观察价A和B更新操作类型
	@param [in] sellPrice 观察价A（卖出参考价）
	@param [in] buyPrice 观察价B（买入参考价）
	@note
	当 B-A >= holdCent 时：如果是常规走势则HOLD，否则GIVE_UP
	当 B-A <= giveUpCent 时：如果是常规走势则GIVE_UP，否则HOLD
	当 giveUpCent < B-A < holdCent 时：OPERATE
	GIVE_UP的含义：
	- 如果当前空仓：当天不买入
	- 如果当前满仓：当天下午卖出（已经买入则第二天早晨卖出）
	*/
	void updateOperate(int32_t sellPrice, int32_t buyPrice);

	/** 开盘检查（割肉或反追）
	@param [in] stock 股票代码
	@param [in] date 交易日期
	@param [in] dayInfo 当天分时数据
	@param [in] chaseCent 反追差价
	@param [in] cutCent 割肉差价
	@return 返回是否执行了交易
	*/
	bool openCheck(const std::string& stock, uint32_t date, const std::vector<int32_t>& dayInfo,
				   int32_t chaseCent, int32_t cutCent);

	/** 时间段检查（割肉或反追）
	@param [in] beginTime 开始时间点
	@param [in] endTime 结束时间点
	@param [in] stock 股票代码
	@param [in] date 交易日期
	@param [in] dayInfo 当天分时数据
	@param [in] chaseCent 反追差价
	@param [in] cutCent 割肉差价
	@param [in] realTrade 是否真实交易（false时仅更新operate）
	@return 返回是否执行了交易
	*/
	bool timeCheck(ObserveTime beginTime, ObserveTime endTime,
				   const std::string& stock, uint32_t date, const std::vector<int32_t>& dayInfo,
				   int32_t chaseCent, int32_t cutCent, bool realTrade);

	/** 挂卖到直卖操作
	@param [in] stock 股票代码
	@param [in] date 交易日期
	@param [in] dayInfo 当天分时数据
	@param [in] chaseCent 反追差价
	@param [in] cutCent 割肉差价
	@param [in] hangSellObserveTime 挂卖时间点
	@param [in] hangSellPrice 挂卖价格
	@param [in] directSellObserveTime 直卖时间点
	@param [in] directSellPrice 直卖价格
	@param [in] realTrade 是否真实交易（false时更新operate和虚拟价格）
	@return 返回最后一次真实或模拟交易时间
	*/
	ObserveTime hangToDirectSell(const std::string& stock, uint32_t date, const std::vector<int32_t>& dayInfo,
						  int32_t chaseCent, int32_t cutCent,
						  ObserveTime hangSellObserveTime, int32_t hangSellPrice,
						  ObserveTime directSellObserveTime, int32_t directSellPrice,
						  bool realTrade);

	/** 挂买到直买操作
	@param [in] stock 股票代码
	@param [in] date 交易日期
	@param [in] dayInfo 当天分时数据
	@param [in] chaseCent 反追差价
	@param [in] cutCent 割肉差价
	@param [in] hangBuyObserveTime 挂买时间点
	@param [in] hangBuyPrice 挂买价格
	@param [in] directBuyObserveTime 直买时间点
	@param [in] directBuyPrice 直买价格
	@param [in] realTrade 是否真实交易（false时更新operate和虚拟价格）
	@return 返回最后一次真实或模拟交易时间
	*/
	ObserveTime hangToDirectBuy(const std::string& stock, uint32_t date, const std::vector<int32_t>& dayInfo,
						 int32_t chaseCent, int32_t cutCent,
						 ObserveTime hangBuyObserveTime, int32_t hangBuyPrice,
						 ObserveTime directBuyObserveTime, int32_t directBuyPrice,
						 bool realTrade);

	/** 更新卖出参数
	@param [in] price 卖出价格
	@param [in] time 卖出时间点
	@param [in] realTrade 是否真实交易（false时仅更新虚拟价格）
	*/
	void updateSellParam(int32_t price, ObserveTime time, bool realTrade);

	/** 更新买入参数
	@param [in] price 买入价格
	@param [in] time 买入时间点
	@param [in] realTrade 是否真实交易（false时仅更新虚拟价格）
	*/
	void updateBuyParam(int32_t price, ObserveTime time, bool realTrade);

private:
	int32_t m_operate; // 操作模式: OPERATE, HOLD, GIVE_UP
	int32_t m_virtualSellPrice; // 虚拟卖出价格（每天计算的参考卖价）
	ObserveTime m_virtualSellObserveTime; // 虚拟卖出时间点
	int32_t m_virtualBuyPrice; // 虚拟买入价格（每天计算的参考买价）
	ObserveTime m_virtualBuyObserveTime; // 虚拟买入时间点
	int32_t m_realSellPrice; // 实际/更新后的卖出起点（用于chase）
	int32_t m_realBuyPrice; // 实际/更新后的买入起点（用于cut）
	bool m_isFull; // 是否满仓
	bool m_isCurrentDayBuy; // 当天是否买入
	bool m_hasFirstBuy; // 是否首次买入
};