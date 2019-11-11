#pragma once
#include "BigNumber/BigNumberAPI.h"
#include <vector>
#include "IntDateTime/IntDateTimeAPI.h"
#include <memory>
#include "StockStrategyMacro.h"

class StockMarket;
/** 策略
*/
class StockStrategyAPI Strategy
{
public:
	/** 从数据库加载hangqing
	@param [in] spMarket hangqing信息
	*/
	void init(const std::shared_ptr<StockMarket>& spMarket);

	/** 加载必要数据
	*/
	virtual void load() = 0;

	/** 单个gupiao是否需要mairu
	@param [in] date 日期
	@param [out] price 价格
	@param [out] date 百分比
	@param [in] buyInfo mairu过的信息
	@return 是否需要mairu
	*/
	virtual bool buy(const IntDateTime& date,
		BigNumber& price,
		BigNumber& percent,
		const std::vector<std::pair<IntDateTime, std::pair<BigNumber, BigNumber>>>& buyInfo) = 0;

	/** 单个gupiao是否需要maichu
	@param [in] date 日期
	@param [out] price 价格
	@param [out] date 百分比
	@param [in] buyInfo mairu过的信息
	@return 是否需要maichu
	*/
	virtual bool sell(const IntDateTime& date,
		BigNumber& price,
		BigNumber& percent,
		const std::vector<std::pair<IntDateTime, std::pair<BigNumber, BigNumber>>>& buyInfo) = 0;

	/** 获取hangqing信息
	@return 返回hangqing信息
	*/
	std::shared_ptr<StockMarket> market();

protected:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::shared_ptr<StockMarket> m_spMarket;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};