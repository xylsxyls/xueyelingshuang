#pragma once
#include "StockFundMacro.h"
#include "BigNumber/BigNumberAPI.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include <memory>
#include <map>

class StockMarket;
/** 股票账户资金管理
*/
class StockFundAPI StockFund
{
public:
	/** 购买股票
	@param [in] date 日期
	@param [in] price 期望的购买价格
	@param [in] stockMarket 股票行情
	@param [in] rate 使用的可用资金比例
	@return 返回是否购买成功，即是否能在期望价格买到
	*/
	bool buyStock(const IntDateTime& date, const BigNumber& price, const std::shared_ptr<StockMarket>& stockMarket, const BigNumber& rate);

	/** 卖出股票
	@param [in] date 日期
	@param [in] price 期望的卖出价格
	@param [in] stock 股票代码
	@param [in] rate 使用的可用资金比例
	@return 返回是否购买成功，即是否能在期望价格买到
	*/
	bool sellStock(const IntDateTime& date, const BigNumber& price, const std::string& stock, const BigNumber& rate);

	/** 增加可用资金
	@param [in] fund 资金
	*/
	void add(const BigNumber& fund);

	/** 冻结可用资金
	@param [in] rate 冻结可用资金比例
	*/
	void freeze(const BigNumber& rate);

	/** 解冻可用资金
	@param [in] rate 解冻可用资金比例
	*/
	void free(const BigNumber& rate);

	/** 总资金
	@param [in] date 日期，该日期可以不是交易日
	@return 返回总资金
	*/
	BigNumber allFund(const IntDateTime& date);

	/** 计算所持股票在指定日期内的涨跌幅，已经放大100倍，包含百分号
	@param [in] stock 股票代码
	@param [in] date 日期
	@param [out] chg 涨跌幅
	@return 返回是否获取成功
	*/
	bool stockChg(const std::string& stock, const IntDateTime& date, BigNumber& chg) const;

	/** 获取该股票的买入时间
	@param [in] stock 股票代码
	@return 返回该股票的买入时间
	*/
	std::vector<IntDateTime> ownedTime(const std::string& stock);

	/** 获取某只股票的买卖情况
	@return 返回某只股票的买卖情况
	*/
	std::vector<std::pair<IntDateTime, std::pair<BigNumber, BigNumber>>> trade(const std::string& stock) const;

	/** 获取买卖日志记录
	@return 返回买卖日志记录
	*/
	std::vector<std::string> stockLog() const;

	/** 目前有几只股票
	@return 返回目前有几只股票
	*/
	BigNumber stockNum() const;

	/** 目前是否持有该股票
	@param [in] stock 股票
	@return 返回目前是否持有该股票
	*/
	bool stockExist(const std::string& stock) const;

	/** 获取持有的股票
	@return 返回持有的股票
	*/
	std::vector<std::string> ownedStock() const;

	/** 是否还有可用资金
	@return 返回是否还有可用资金
	*/
	bool hasAvailableFund() const;

	/** 是否有冻结资金
	@return 返回是否有冻结资金
	*/
	bool hasFreezeFund() const;

private:
	//可用资金
	BigNumber m_available;
	//冻结资金
	BigNumber m_freeze;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	//股票代码，购买日期（成本资金，股票仓位）组
	std::map<std::string, std::vector<std::pair<IntDateTime, std::pair<BigNumber, BigNumber>>>> m_stock;
	//股票行情
	std::map<std::string, std::shared_ptr<StockMarket>> m_stockMarket;
	//买卖日志记录
	std::vector<std::string> m_stockLog;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	//总资金
	BigNumber m_all;
};