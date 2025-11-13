#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <utility>
#include "BigNumber/BigNumberAPI.h"
#include "StockCharge/StockChargeAPI.h"
#include "Market.h"
#include "Config.h"

// 前置声明
class Market;

/** 持仓信息结构体
*/
struct Position
{
	int32_t m_price;      // 交易价格（单位：分）
	int32_t m_shares;     // 交易股数
	uint32_t m_date;      // 交易日期
	ObserveTime m_time;   // 交易时间点

	Position();
};

/** 交易信息结构体
*/
struct Trade : public Position
{
	int32_t m_fee;        // 交易手续费（单位：分）

	Trade();
	bool empty() const;
};

/** 做T操作记录结构体（只包含完整的T交易）
*/
struct TOperation
{
	std::vector<Trade> m_sellTrade;                   // 卖出交易记录
	std::vector<Trade> m_buyTrade;                    // 买入交易记录

	int32_t sellShares() const;                       // 卖出股数
	int32_t buyShares() const;                        // 买入股数
	int32_t diff() const;                             // 价格差价（卖出-买入，单位：分）
	int32_t totalFee() const;                         // 总手续费（买入+卖出，单位：分）
	int32_t totalProfit(int32_t endPrice) const;      // 净收益（差价收益-手续费，单位：分）
};

/** 完整交易记录结构体
*/
struct CompleteTrade
{
	Trade m_buyTrade;                             // 买入交易记录
	Trade m_sellTrade;                            // 卖出交易记录
	std::vector<TOperation> m_vecTOperations;     // 做T操作记录
	TOperation m_incompleteTOperation;            // 最后一次不完整的T操作记录

	int32_t allTDiff() const;                          // 做T价格差价总和（单位：分）
	int32_t allTFee() const;                           // 做T总手续费（单位：分）
	int32_t allTProfit(int32_t endPrice) const;        // 做T总净收益（单位：分）     
	int32_t tradeProfit() const;                       // 交易总净收益（单位：分）
	int32_t beginEndFee() const;                       // 开始结束交易手续费（单位：分）
};

/** 资金账户类
- 管理交易资金、持仓状态，执行买卖操作，计算账户净值
- 通过交易手数区分做T操作和结束交易
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
	bool buyAll(const std::string& stock, int32_t price, uint32_t date, ObserveTime time);

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
	bool sellAllForT(const std::string& stock, int32_t price, uint32_t date, ObserveTime time);

	/** 获取账户总资产（现金+股票市值）
	@param [in] date 当前日期
	@param [in] time 当前时间点，传入默认值则表示当天收盘价
	@return 返回总资产金额（单位：分）
	*/
	int32_t getTotalValue(uint32_t date, ObserveTime time = ObserveTime::COUNT);

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
	std::shared_ptr<Position> getPosition(const std::string& stock) const;

	/** 获取所有持仓信息
	@return 返回持仓列表
	*/
	const std::map<std::string, std::shared_ptr<Position>>& getAllPositions() const;

	/** 获取完整交易记录
	@return 返回完整交易记录映射（股票代码->交易记录列表）
	*/
	const std::map<std::string, std::vector<CompleteTrade>>& getCompleteTrades() const;

	/** 输出当前账户的所有操作记录
	@return 返回操作记录描述字符串向量
	*/
	std::vector<std::string> exportTradeRecords() const;

	/** 清空所有持仓和重置资金
	*/
	void reset();

	/** 强制关闭所有未完成交易（用于回测结束）
	@param [in] date 关闭日期
	@param [in] time 关闭时间点
	*/
	void closeAllTrades(uint32_t date, ObserveTime time = ObserveTime::COUNT);

	/** 获取所有交易的做T价格差价总和
	@return 返回做T价格差价总和（单位：分）
	*/
	int32_t allTDiff() const;

	/** 获取所有交易的做T总手续费
	@return 返回做T总手续费（单位：分）
	*/
	int32_t allTFee() const;

	/** 获取所有交易的做T总净收益
	@param [in] 最后一天日期
	@return 返回做T总净收益（单位：分）
	*/
	int32_t allTProfit(uint32_t lastDate) const;

	/** 获取所有交易的交易总净收益
	@return 返回交易总净收益（单位：分）
	*/
	int32_t tradeProfit() const;

	/** 获取最开始和最后一笔结束交易的总手续费
	@return 返回手续费（单位：分）
	*/
	int32_t allBeginEndFee() const;

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
	@param [in] fee 交易手续费（单位：分）
	@param [in] isFullPosition 是否为全仓买入
	@return 返回买入是否成功
	*/
	bool executeBuy(const std::string& stock, int32_t price, int32_t shares, uint32_t date, ObserveTime time, int32_t fee, bool isFullPosition = true);

	/** 执行卖出操作
	@param [in] stock 股票代码
	@param [in] price 卖出价格（单位：分）
	@param [in] shares 卖出股数
	@param [in] date 交易日期
	@param [in] time 交易时间点
	@param [in] fee 交易手续费（单位：分）
	@param [in] isTOperation 是否为做T操作
	@return 返回卖出是否成功
	*/
	bool executeSell(const std::string& stock, int32_t price, int32_t shares, uint32_t date, ObserveTime time, int32_t fee, bool isTOperation = false);

	/** 处理做T操作
	@param [in] stock 股票代码
	@param [in] sellTrade 卖出交易记录
	*/
	void processTOperations(const std::string& stock, const Trade& sellTrade);

	/** 处理全仓买入操作
	@param [in] stock 股票代码
	@param [in] buyTrade 买入交易记录
	*/
	void processFullBuy(const std::string& stock, const Trade& buyTrade);

	/** 处理结束交易操作
	@param [in] stock 股票代码
	@param [in] sellTrade 卖出交易记录
	*/
	void processCompleteTrade(const std::string& stock, const Trade& sellTrade);

	/** 从市场数据获取股票价格
	@param [in] stock 股票代码
	@param [in] date 日期
	@param [in] time 时间点
	@return 返回股票价格，如果找不到返回0
	*/
	int32_t getStockPriceFromMarket(const std::string& stock, uint32_t date, ObserveTime time) const;

	/** 获取当前进行中的交易（未完成的交易）
	@param [in] stock 股票代码
	@return 返回交易记录指针，如果不存在返回nullptr
	*/
	CompleteTrade* getCurrentTrade(const std::string& stock);

private:
	// 初始资金（单位：分），仅用于记录和计算收益率
	int32_t m_initialFund;
	// 可用资金（单位：分），当前可用的现金
	int32_t m_availableFund;
	// 持仓映射，记录当前持有的股票
	std::map<std::string, std::shared_ptr<Position>> m_positions;
	// 市场数据，用于获取股票价格
	std::shared_ptr<Market> m_spMarket;
	// 完整交易记录映射（股票代码->交易记录列表）
	std::map<std::string, std::vector<CompleteTrade>> m_completeTrades;
	// 待匹配的做T操作记录
	std::map<std::string, TOperation> m_pendingTOperations;
};