#pragma once
#include "StockFundMacro.h"
#include "BigNumber/BigNumberAPI.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include <memory>
#include <map>

class StockDay;
/** gupiao账户zijin管理
*/
class StockFundAPI StockFund
{
public:
	/** 购买gupiao
	@param [in] price 期望的购买jiage
	@param [in] rate 使用的可用zijin比例，0-1
	@param [in] spStockDay gupiao当天hangqing
	@return 返回是否购买成功，即是否能在期望jiage买到
	*/
	bool buyStock(const BigNumber& price, const BigNumber& rate, const std::shared_ptr<StockDay>& spStockDay, int32_t strategyType = 0);

	/** 卖出gupiao
	@param [in] price 期望的卖出jiage
	@param [in] rate 卖出比例，0-1
	@param [in] spStockDay gupiao当天hangqing
	@return 返回是否卖出成功，即是否能在期望jiage卖出
	*/
	bool sellStock(const BigNumber& price, const BigNumber& rate, const std::shared_ptr<StockDay>& spStockDay);

	/** 增加可用zijin
	@param [in] fund zijin
	*/
	void add(const BigNumber& fund);

	/** 冻结可用zijin
	@param [in] rate 冻结可用zijin比例
	*/
	void freeze(const BigNumber& rate);

	/** 解冻可用zijin
	@param [in] rate 解冻可用zijin比例
	*/
	void free(const BigNumber& rate);

	/** 总zijin
	@param [in] allOwnedStockDayData 包含数据的日期结构体
	@return 返回总zijin
	*/
	BigNumber allFund(const std::map<std::string, std::shared_ptr<StockDay>>& allOwnedStockDayData);

	/** 计算所持gupiao在指定日期内的zhangdiefu，已经放大100倍，包含百分号
	@param [in] stock gupiao代码
	@param [in] dayDate 包含数据的日期结构体
	@param [out] chg zhangdiefu
	@return 返回是否获取成功
	*/
	bool stockChg(const std::string& stock, const std::shared_ptr<StockDay>& dateDay, BigNumber& chg) const;

	/** 获取第一次mairu的时间
	@param [in] stock gupiao代码
	@return 返回第一次mairu的时间
	*/
	IntDateTime firstBuyDate(const std::string& stock);

	/** 获取该gupiao的买入时间
	@param [in] stock gupiao代码
	@return 返回该gupiao的买入时间
	*/
	std::vector<IntDateTime> ownedTime(const std::string& stock) const;

	/** 获取某只gupiao的买卖情况
	@return 返回某只gupiao的买卖情况
	*/
	std::vector<std::pair<IntDateTime, std::pair<BigNumber, BigNumber>>> trade(const std::string& stock) const;

	/** 获取买卖日志记录
	@return 返回买卖日志记录
	*/
	std::vector<std::string> stockLog() const;

	/** 打印买卖日志记录
	*/
	void printStockLog() const;

	/** 获取买卖数据日志
	@return 返回买卖数据日志
	*/
	std::vector<std::pair<std::vector<BigNumber>, std::vector<IntDateTime>>> dataLog() const;

	/** 清空zichan
	*/
	void clear();

	/** 目前有几只gupiao
	@return 返回目前有几只gupiao
	*/
	BigNumber stockNum() const;

	/** 目前是否持有该gupiao
	@param [in] stock gupiao
	@return 返回目前是否持有该gupiao
	*/
	bool stockExist(const std::string& stock) const;

	/** 获取持有的gupiao
	@return 返回持有的gupiao
	*/
	std::vector<std::string> ownedStock() const;

	/** 获取gupiao对应策略
	@return 返回gupiao对应策略
	*/
	int32_t stockStrategy(const std::string& stock) const;

	/** 获取可用zijin
	@return 返回可用zijin
	*/
	BigNumber availableFund() const;

	/** 是否还有可用zijin
	@return 返回是否还有可用zijin
	*/
	bool hasAvailableFund() const;

	/** 是否有冻结zijin
	@return 返回是否有冻结zijin
	*/
	bool hasFreezeFund() const;

	/** 获取goumai信息
	@param [in] stock gupiao代码
	@param [out] buyInfo goumai信息
	*/
	void buyInfo(const std::string& stock, std::vector<std::pair<IntDateTime, std::pair<BigNumber, BigNumber>>>& buyInfo);

	/** 获取所有goumai信息
	@return 所有goumai信息
	*/
	std::map<std::string, std::vector<std::pair<IntDateTime, std::pair<BigNumber, BigNumber>>>>* allBuyInfo();

private:
	//可用zijin
	BigNumber m_available;
	//冻结zijin
	BigNumber m_freeze;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	//gupiao代码，购买日期（成本zijin，gupiaocangwei）组
	std::map<std::string, std::vector<std::pair<IntDateTime, std::pair<BigNumber, BigNumber>>>> m_stock;
	//gupiao代码，策略
	std::map<std::string, int32_t> m_stockStrategy;
	//gupiaohangqing
	//std::map<std::string, std::shared_ptr<StockMarket>> m_stockMarket;
	//买卖日志记录
	std::vector<std::string> m_stockLog;
	//买卖记录，gupiao代码，shouyi率，/*cangwei比*/，chigu天数，买入时间，卖出时间
	std::vector<std::pair<std::vector<BigNumber>, std::vector<IntDateTime>>> m_dataLog;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	//总zijin
	BigNumber m_all;
};