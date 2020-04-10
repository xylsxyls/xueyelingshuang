#pragma once
#include "Solution.h"
#include <memory>

class Strategy;
class AvgFundHighScore;
struct SolutionInfo;
class StockSolutionAPI ObserveStrategy : public Solution
{
public:
	/** 构造函数
	*/
	ObserveStrategy();

public:
	/** 初始化
	@param [in] strategyType 子类型
	@param [in] calcDays 计算天数
	*/
	void init(StrategyType strategyType, int32_t calcDays);

	/** 设置参数
	@param [in] solutionInfo 解决方案信息
	*/
	void setSolutionInfo(const std::shared_ptr<SolutionInfo>& solutionInfo);

	/** 选出可以goumai的gupiao
	@param [out] buyStock 选出的gupiao集合，stock,price,rate0-1
	@param [in] date 日期
	@return 是否有选出的gupiao，集合不为0不代表有goumai的gupiao
	*/
	virtual bool buy(std::vector<std::pair<std::string, StockInfo>>& buyStock, const IntDateTime& date);

	/** 询问单只gupiao需不需要maichu
	@param [in] sellStock maichu的gupiao集合，stock,price,rate0-1
	@param [in] date 日期
	@return 返回是否有需要maichu的gupiao，集合不为0不代表有maichu的gupiao
	*/
	virtual bool sell(std::vector<std::pair<std::string, StockInfo>>& sellStock, const IntDateTime& date);

protected:
	/** 获取向前日期
	@param [in] stock gupiao代码
	@param [in] date 日期
	@param [in] days 向前的天数
	@return 返回向前日期
	*/
	IntDateTime getBeforeDay(const std::string& stock, const IntDateTime& date, int32_t days);

	/** 获取对应的hangqing信息
	@param [in] stock gupiao代码
	@return 返回hangqing信息
	*/
	std::shared_ptr<StockMarket> getMarket(const std::string& stock);

protected:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::shared_ptr<AvgFundHighScore> m_avgSolution;
	StrategyType m_strategyType;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	int32_t m_stockNum;
	int32_t m_calcDays;
};