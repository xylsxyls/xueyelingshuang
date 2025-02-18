#pragma once
#include "Solution.h"
#include <memory>

class Strategy;
class AvgFundHighScore;
struct SolutionInfo;
class StockStorageBase;
class StockSolutionAPI ObserveStrategy : public Solution
{
public:
	/** 构造函数
	*/
	ObserveStrategy();

public:
	/** 初始化
	@param [in] stockNum 允许goumai的gupiao个数
	@param [in] calcDays 计算天数
	@param [in] storage 仓库
	*/
	void init(int32_t stockNum, int32_t calcDays, StockStorageBase* storage);

	/** 设置使用的方案
	@param [in] spSolution 使用的方案
	*/
	void setSolution(const std::shared_ptr<Solution>& spSolution);

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

	/** 获取计算天数
	@return 返回计算天数
	*/
	int32_t calcDays();

protected:
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
	std::map<SolutionType, std::shared_ptr<Solution>> m_solutionMap;
	std::shared_ptr<Solution> m_useSolution;
	StrategyType m_strategyType;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	int32_t m_stockNum;
	int32_t m_calcDays;
	StockStorageBase* m_storage;
};