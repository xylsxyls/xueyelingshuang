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

	DISPOSABLE_STRATEGY
};

struct SolutionInfo;
class StockSolutionAPI Solution
{
public:
	/** 设置策略
	@param [in] spStrategy 策略，要求两个，第一个用于策略，第二个用于查看通过计数
	*/
	void init(const std::vector<std::shared_ptr<Strategy>>& vecStrategy);

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

protected:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::vector<std::shared_ptr<Strategy>> m_vecStrategy;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	SolutionType m_solutionType;
};