#pragma once
#include "BigNumber/BigNumberAPI.h"
#include <vector>
#include "IntDateTime/IntDateTimeAPI.h"
#include <memory>
#include "StockStrategyMacro.h"
#include "StrategyInfo.h"
#include "StockInfo.h"

enum StrategyType
{
	STRATEGY_INIT,

	SAR_RISE_BACK_COUNT,

	SAR_RISE_BACK,

	CATCH_UP,

	SAR_RISE_BACK_THIRTY_LINE,

	LINE_BACK,

	STRATEGY_TYPE_SIZE
};

/** 策略
*/
class StockStrategyAPI Strategy
{
public:
	/** 构造函数
	*/
	Strategy();

public:
	/** 设置策略信息
	@param [in] strategyInfo 策略信息
	*/
	void setStrategyInfo(const std::shared_ptr<StrategyInfo>& strategyInfo);

	/** 单个gupiao是否需要mairu
	@param [in] date 日期
	@param [in] stockInfo 选中信息
	@return 是否需要mairu
	*/
	virtual bool buy(const IntDateTime& date, StockInfo& stockInfo) = 0;

	/** 单个gupiao是否需要maichu
	@param [in] date 日期
	@param [in] stockInfo 选中信息
	@return 是否需要maichu
	*/
	virtual bool sell(const IntDateTime& date, StockInfo& stockInfo);

	/** 观察类型单个gupiao是否需要maichu
	@param [in] date 日期
	@param [in] stockInfo 选中信息
	@return 是否需要maichu
	*/
	virtual bool observeSell(const IntDateTime& date, StockInfo& stockInfo);

	/** 获取策略类型
	@return 返回策略类型
	*/
	StrategyType type();

protected:
	StrategyType m_strategyType;

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::shared_ptr<StrategyInfo> m_strategyInfo;
#ifdef _MSC_VER
#pragma warning(pop)
#endif

};