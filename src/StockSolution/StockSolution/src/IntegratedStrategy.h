#pragma once
#include "Solution.h"
#include <memory>

class Strategy;
class AvgFundHighScore;
struct SolutionInfo;
class StockSolutionAPI IntegratedStrategy : public Solution
{
public:
	/** 构造函数
	*/
	IntegratedStrategy();

public:
	/** 初始化
	@param [in] stockNum gupiao个数
	@param [in] vecStrategyType 执行类型
	*/
	void init(int32_t stockNum, const std::vector<StrategyType>& vecStrategyType);

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

protected:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::shared_ptr<AvgFundHighScore> m_avgSolution;
	std::vector<StrategyType> m_vecStrategyType;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};