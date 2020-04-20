#pragma once
#include "StockTypeMacro.h"
#include "BigNumber/BigNumberAPI.h"
#include <vector>
#include <memory>
#include "IntDateTime/IntDateTimeAPI.h"

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

enum SolutionType
{
	SOLUTION_INIT,

	AVG_FUND_HIGH_SCORE,

	STRATEGY_SET,

	DISPOSABLE_STRATEGY,

	INTEGRATED_STRATEGY,

	OBSERVE_STRATEGY
};

struct StockTypeAPI StockInfo
{
	//价格
	BigNumber m_price;
	//百分比 0-100
	BigNumber m_percent;
	//分数
	BigNumber m_score;
	//比例 0-1
	BigNumber m_rate;
};

struct StockTypeAPI ChooseParam
{
	//使用的类型
	StrategyType m_useType;
	//使用的计数类型
	StrategyType m_useCountType;
	//是否是观察类型
	bool m_isObserve;
	//解决方案类型，如果是observer则是内部类型，不是则直接使用
	SolutionType m_solutionType;

	/** 构造函数
	*/
	ChooseParam();
};

class Solution;
class Strategy;
struct SolutionInfo;
struct StrategyInfo;
class StockMarket;

class StockTypeAPI StockStorageBase
{
public:
	virtual std::vector<std::string>* filterStock(const IntDateTime& date) = 0;

	virtual std::shared_ptr<Solution> solution(SolutionType solutionType) = 0;

	virtual std::shared_ptr<Strategy> strategy(StrategyType strategyType) = 0;

	virtual std::shared_ptr<SolutionInfo> solutionInfo(SolutionType solutionType) = 0;

	virtual std::shared_ptr<StrategyInfo> strategyInfo(StrategyType solutionType, const std::string& stock) = 0;

	virtual std::shared_ptr<StockMarket> market(const std::string& stock) = 0;
};