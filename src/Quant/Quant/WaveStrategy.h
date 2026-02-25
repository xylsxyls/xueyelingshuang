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

	/** 填入并检测参数是否合理
	@return 返回是否合理
	*/
	bool fillCheckParam();

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
	void updateOperate();

	/** 开盘检查（割肉或反追）
	@param [in] stock 股票代码
	@return 返回是否执行了交易
	*/
	bool openCheck(const std::string& stock);

	/** 时间段检查（割肉或反追）
	@param [in] beginTime 开始时间点
	@param [in] endTime 结束时间点
	@param [in] stock 股票代码
	@param [in] dayInfo 当天分时数据
	@return 返回是否执行了交易
	*/
	bool timeCheck(ObserveTime beginTime, ObserveTime endTime,
		const std::string& stock, const std::vector<int32_t>& dayInfo);

	/** 挂卖到直卖操作
	@param [in] stock 股票代码
	@param [in] dayInfo 当天分时数据
	@return 返回最后一次真实或模拟交易时间
	*/
	ObserveTime hangToDirectSell(const std::string& stock, const std::vector<int32_t>& dayInfo);

	/** 挂卖到直卖的检测买入
	@param [in] stock 股票代码
	@param [in] dayInfo 当天分时数据
	@param [in] current 当前时间
	@return 返回实际交易时间
	*/
	ObserveTime checkSell(const std::string& stock, const std::vector<int32_t>& dayInfo, ObserveTime current);

	/** 挂买到直买操作
	@param [in] stock 股票代码
	@param [in] dayInfo 当天分时数据
	@return 返回最后一次真实或模拟交易时间
	*/
	ObserveTime hangToDirectBuy(const std::string& stock, const std::vector<int32_t>& dayInfo);

	/** 挂买到直买的检测买入
	@param [in] stock 股票代码
	@param [in] dayInfo 当天分时数据
	@param [in] current 当前时间
	@return 返回实际交易时间
	*/
	ObserveTime checkBuy(const std::string& stock, const std::vector<int32_t>& dayInfo, ObserveTime current);

	/** 更新卖出参数
	@param [in] price 卖出价格
	@param [in] time 卖出时间点
	@param [in] isRealTrade 是否是真实交易
	*/
	void updateSellParam(int32_t price, ObserveTime time, bool isRealTrade);

	/** 更新买入参数
	@param [in] price 买入价格
	@param [in] time 买入时间点
	@param [in] isRealTrade 是否是真实交易
	*/
	void updateBuyParam(int32_t price, ObserveTime time, bool isRealTrade);

	/** 反追价
	@return 返回反追价
	*/
	int32_t chasePrice();

	/** 割肉价
	@return 返回割肉价
	*/
	int32_t cutPrice();

private:
	// 当天日期
	uint32_t m_date;
	// 开盘价（9:25数据）
	int32_t m_openPrice;
	// 收盘价
	int32_t m_closePrice;
	// 挂卖价
	int32_t m_hangSellPrice;
	// 挂卖价时间点
	ObserveTime m_hangSellObserveTime;
	// 直卖价
	int32_t m_directSellPrice;
	// 直卖价时间点
	ObserveTime m_directSellObserveTime;
	// 挂买价
	int32_t m_hangBuyPrice;
	// 挂买价时间点
	ObserveTime m_hangBuyObserveTime;
	// 直买价
	int32_t m_directBuyPrice;
	// 直买价时间点
	ObserveTime m_directBuyObserveTime;
	// 放弃挂卖价
	int32_t m_giveUpHangSellPrice;
	// 放弃挂卖价时间点
	ObserveTime m_giveUpHangSellObserveTime;
	// 放弃直卖价
	int32_t m_giveUpDirectSellPrice;
	// 放弃直卖价时间点
	ObserveTime m_giveUpDirectSellObserveTime;
	// 观察价A时间点
	ObserveTime m_aObserveTime;
	// 观察价B时间点
	ObserveTime m_bObserveTime;
	// 观察价B-A大于等于多少时当天直买，第二天早上不卖
	int32_t m_holdCent;
	// 观察价B-A小于等于多少时，当天不买，如果当前持有则当天下午卖出，如果已经买入则第二天早晨卖出
	int32_t m_giveUpCent;
	// 观察价A
	int32_t m_aPrice;
	// 观察价B
	int32_t m_bPrice;
	// 当天早上卖出后反追差价
	int32_t m_chaseCent;
	// 割肉差价
	int32_t m_cutCent;
	// 是否为常规走势
	bool m_isNormal;
	// 当天是否可以交易
	bool m_canTrade;
	// 是否首次买入
	bool m_hasFirstBuy;
	// 当天是否已经更新买入参数
	bool m_isUpdateBuyParam;
	// 当天是否已经更新买入参数
	bool m_isUpdateSellParam;
};