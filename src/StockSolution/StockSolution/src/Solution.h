#pragma once
#include "StockSolutionMacro.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include "BigNumber/BigNumberAPI.h"
#include <map>
#include <memory>
#include "StockStrategy/StockStrategyAPI.h"

enum SolutionType
{
	SOLUTION_INIT,

	AVG_FUND_HIGH_SCORE,

	STRATEGY_SET,

	DISPOSABLE_STRATEGY,

	INTEGRATED_STRATEGY,

	OBSERVE_STRATEGY
};

struct SolutionInfo;
class StockSolutionAPI Solution
{
public:
	/** 设置策略
	@param [in] spStrategy 策略，要求两个，第一个用于策略，第二个用于查看通过计数
	*/
	void init(const std::map<StrategyType, std::pair<std::shared_ptr<Strategy>, std::shared_ptr<Strategy>>>& vecStrategy);

	/** 设置参数
	@param [in] useType 使用类型
	*/
	void setParam(StrategyType useType);

	/** 选出可以goumai的gupiao
	@param [out] buyStock 选出的gupiao集合，stock,price,rate0-1
	@param [in] date 日期
	@param [in] solutionInfo 解决方案需要的信息
	@return 是否有选出的gupiao
	*/
	virtual bool buy(std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>& buyStock,
		const IntDateTime& date,
		const std::shared_ptr<SolutionInfo>& solutionInfo);

	/** 询问单只gupiao需不需要maichu
	@param [in] sellStock maichu的gupiao集合，stock,price,rate0-1
	@param [in] date 日期
	@param [in] solutionInfo 解决方案需要的信息
	@return 返回是否有需要maichu的gupiao
	*/
	virtual bool sell(std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>& sellStock,
		const IntDateTime& date,
		const std::shared_ptr<SolutionInfo>& solutionInfo);

	/** 获取解决方案类型
	@return 返回解决方案类型
	*/
	SolutionType type();

	/** 获取执行的策略
	@param [in] useType 使用的类型
	@return 返回执行的策略
	*/
	std::shared_ptr<Strategy> strategy(StrategyType useType);

	/** 获取执行计数的策略
	@param [in] useType 使用的类型
	@return 返回执行计数的策略
	*/
	std::shared_ptr<Strategy> strategyCount(StrategyType useType);

	/** 获取最小通过个数
	@param [in] solutionInfo 解决方案需要的信息
	@return 返回最小通过个数
	*/
	int32_t minPollSize(const std::shared_ptr<SolutionInfo>& solutionInfo);

	/** 获取去掉个数
	@param [in] solutionInfo 解决方案需要的信息
	@return 返回去掉个数
	*/
	int32_t popSize(int32_t buySize, const std::shared_ptr<SolutionInfo>& solutionInfo);

protected:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	StrategyType m_useType;
	std::map<StrategyType, std::pair<std::shared_ptr<Strategy>, std::shared_ptr<Strategy>>> m_mapStrategy;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	SolutionType m_solutionType;
};