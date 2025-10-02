#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>
#include "BigNumber/BigNumberAPI.h"
#include "StockCharge/StockChargeAPI.h"
#include "Market.h"
#include "Config.h"

// 前置声明
class Market;

/** 做T操作记录结构体
*/
struct TOperation
{
	uint32_t sellDate;          // 卖出日期
	ObserveTime sellTime;       // 卖出时间点
	int32_t sellPrice;          // 卖出价格（单位：分）
	int32_t sellFee;            // 卖出手续费（单位：分）

	uint32_t buyDate;           // 买入日期
	ObserveTime buyTime;        // 买入时间点
	int32_t buyPrice;           // 买入价格（单位：分）
	int32_t buyFee;             // 买入手续费（单位：分）

	int32_t shares;             // 做T股数
	int32_t priceDiff;          // 价格差价（卖出-买入，单位：分）
	int32_t totalFee;           // 总手续费（买入+卖出，单位：分）
	int32_t netProfit;          // 净收益（差价收益-手续费，单位：分）
};

/** 完整交易记录结构体
*/
struct CompleteTrade
{
	uint32_t buyDate;           // 买入日期
	ObserveTime buyTime;        // 买入时间点
	int32_t buyPrice;           // 买入价格（单位：分）
	int32_t buyShares;          // 买入股数
	int32_t buyFee;             // 买入手续费（单位：分）

	uint32_t sellDate;          // 卖出日期（0表示未卖出）
	ObserveTime sellTime;       // 卖出时间点
	int32_t sellPrice;          // 卖出价格（单位：分）
	int32_t sellShares;         // 卖出股数
	int32_t sellFee;            // 卖出手续费（单位：分）

	std::vector<TOperation> tOperations; // 做T操作记录
	int32_t totalTDiff;         // 做T价格差价总和（单位：分）
	int32_t totalTFee;          // 做T总手续费（单位：分）
	int32_t totalTProfit;       // 做T总净收益（单位：分）
	int32_t tradeProfit;        // 交易总净收益（单位：分）

	bool isClosed;              // 交易是否已结束（已卖出）
};

/** 持仓信息结构体
*/
struct Position
{
	std::string stock;         // 股票代码
	int32_t shares;            // 持有股数
	int32_t buyPrice;          // 买入价格（单位：分）
	uint32_t buyDate;          // 买入日期
	ObserveTime buyTime;       // 买入时间点
};

/** 简单交易记录结构体（用于临时记录）
*/
struct SimpleTradeRecord
{
	uint32_t date;             // 交易日期
	ObserveTime time;          // 交易时间点
	std::string stock;         // 股票代码
	std::string operation;     // 操作类型（BUY/SELL）
	int32_t price;             // 交易价格（单位：分）
	int32_t shares;            // 交易股数
	int32_t fee;               // 手续费（单位：分）
};

/** 资金账户类
* 管理交易资金、持仓状态，执行买卖操作，计算账户净值
* 通过交易手数区分做T操作和结束交易
*/
class Fund
{
public:
	/** 构造函数
	*/
	Fund();

	/** 初始化资金账户
	@param [in] fund 初始资金金额（单位：分）
	@return 返回初始化是否成功
	*/
	bool init(int32_t fund);

	/** 设置市场行情数据
	@param [in] spMarket 市场数据共享指针
	*/
	void setMarket(const std::shared_ptr<Market>& spMarket);

	/** 全仓买入指定股票
	@param [in] stock 股票代码
	@param [in] price 买入价格（单位：分）
	@param [in] date 交易日期
	@param [in] time 交易时间点
	@return 返回买入是否成功
	*/
	bool buyFullPosition(const std::string& stock, int32_t price, uint32_t date, ObserveTime time);

	/** 卖出指定股票的全部持仓（结束交易）
	@param [in] stock 股票代码
	@param [in] price 卖出价格（单位：分）
	@param [in] date 交易日期
	@param [in] time 交易时间点
	@return 返回卖出是否成功
	*/
	bool sellAll(const std::string& stock, int32_t price, uint32_t date, ObserveTime time);

	/** 做T卖出：卖出所有手数减1手（保留底仓）
	@param [in] stock 股票代码
	@param [in] price 卖出价格（单位：分）
	@param [in] date 交易日期
	@param [in] time 交易时间点
	@return 返回卖出是否成功
	*/
	bool sellForT(const std::string& stock, int32_t price, uint32_t date, ObserveTime time);

	/** 做T买入：使用全部可用资金买入
	@param [in] stock 股票代码
	@param [in] price 买入价格（单位：分）
	@param [in] date 交易日期
	@param [in] time 交易时间点
	@return 返回买入是否成功
	*/
	bool buyForT(const std::string& stock, int32_t price, uint32_t date, ObserveTime time);

	/** 获取账户总资产（现金+股票市值）
	@param [in] date 当前日期
	@param [in] time 当前时间点
	@return 返回总资产金额（单位：分）
	*/
	int32_t getTotalValue(uint32_t date, ObserveTime time = ObserveTime::TIME1410);

	/** 获取可用资金
	@return 返回可用资金金额（单位：分）
	*/
	int32_t getAvailableFund() const;

	/** 获取初始资金
	@return 返回初始资金金额（单位：分）
	*/
	int32_t getInitialFund() const;

	/** 获取指定股票的持仓信息
	@param [in] stock 股票代码
	@return 返回持仓信息指针，如果未持有返回nullptr
	*/
	const Position* getPosition(const std::string& stock) const;

	/** 获取所有持仓信息
	@return 返回持仓列表
	*/
	const std::vector<Position>& getAllPositions() const;

	/** 获取完整交易记录
	@return 返回完整交易记录映射（股票代码->交易记录列表）
	*/
	const std::map<std::string, std::vector<CompleteTrade>>& getCompleteTrades() const;

	/** 清空所有持仓和重置资金
	*/
	void reset();

	/** 强制关闭所有未完成交易（用于回测结束）
	@param [in] date 关闭日期
	@param [in] time 关闭时间点
	*/
	void closeAllTrades(uint32_t date, ObserveTime time);

private:
	/** 计算指定价格下最大可买入股数（考虑手续费）
	@param [in] stock 股票代码
	@param [in] price 买入价格（单位：分）
	@return 返回最大可买入股数
	*/
	int32_t calculateMaxShares(const std::string& stock, int32_t price) const;

	/** 执行买入操作
	@param [in] stock 股票代码
	@param [in] price 买入价格（单位：分）
	@param [in] shares 买入股数
	@param [in] date 交易日期
	@param [in] time 交易时间点
	@return 返回买入是否成功
	*/
	bool executeBuy(const std::string& stock, int32_t price, int32_t shares, uint32_t date, ObserveTime time);

	/** 执行卖出操作
	@param [in] stock 股票代码
	@param [in] price 卖出价格（单位：分）
	@param [in] shares 卖出股数
	@param [in] date 交易日期
	@param [in] time 交易时间点
	@param [in] isTOperation 是否为做T操作
	@return 返回卖出是否成功
	*/
	bool executeSell(const std::string& stock, int32_t price, int32_t shares, uint32_t date, ObserveTime time, bool isTOperation = false);

	/** 记录交易操作
	@param [in] record 简单交易记录
	*/
	void recordTrade(const SimpleTradeRecord& record);

	/** 处理做T操作
	@param [in] stock 股票代码
	@param [in] sellRecord 卖出记录
	*/
	void processTOperations(const std::string& stock, const SimpleTradeRecord& sellRecord);

	/** 从市场数据获取股票价格
	@param [in] stock 股票代码
	@param [in] date 日期
	@param [in] time 时间点
	@return 返回股票价格，如果找不到返回0
	*/
	int32_t getStockPriceFromMarket(const std::string& stock, uint32_t date, ObserveTime time) const;

private:
	// 初始资金（单位：分），仅用于记录和计算收益率
	int32_t m_initialFund;

	// 可用资金（单位：分），当前可用的现金
	int32_t m_availableFund;

	// 持仓列表，记录当前持有的股票
	std::vector<Position> m_positions;

	// 市场数据，用于获取股票价格
	std::shared_ptr<Market> m_spMarket;

	// 完整交易记录映射（股票代码->交易记录列表）
	std::map<std::string, std::vector<CompleteTrade>> m_completeTrades;

	// 待匹配的交易记录（用于做T检测）
	std::vector<SimpleTradeRecord> m_pendingTrades;
};