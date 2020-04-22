#pragma once
#include "IntDateTime/IntDateTimeAPI.h"
#include "BigNumber/BigNumberAPI.h"
#include <map>
#include <memory>
#include <set>
#include "StockStrategyMacro.h"
#include "StockIndicator/StockIndicatorAPI.h"

class StockFund;
class StockMarket;
/** 策略信息，跟随单只gupiao
*/
struct StockStrategyAPI StrategyInfo
{
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	//hangqing
	std::shared_ptr<StockMarket> m_spMarket;
	//zhibiao
	std::map<std::string, std::shared_ptr<IndicatorManagerBase>> m_spIndicator;
	//执行的zhanghu
	StockFund* m_fund;
#ifdef _MSC_VER
#pragma warning(pop)
#endif

	StrategyInfo()
	{
		m_spMarket = nullptr;
		m_fund = nullptr;
	}

	virtual ~StrategyInfo()
	{

	}

	/** 最小通过个数
	@return 返回最小通过个数
	*/
	virtual int32_t minPollSize() = 0;

	/** 过滤个数
	@return 返回过滤个数
	*/
	virtual int32_t popSize(int32_t buySize) = 0;

	/** 选择个数
	@return 返回选择个数
	*/
	virtual int32_t stockNum() = 0;

	/** 需要的zhibiao
	@return 返回需要的zhibiao
	*/
	virtual std::set<std::string> needIndicator() = 0;

	/** 获取需要提前的jiaoyiri天数
	@return 返回需要提前的jiaoyiri天数
	*/
	virtual int32_t needMoveDay() = 0;
};