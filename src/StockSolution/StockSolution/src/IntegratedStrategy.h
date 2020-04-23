#pragma once
#include "Solution.h"
#include <memory>
#include "StockType/StockTypeAPI.h"
#include <map>

class Strategy;
class AvgFundHighScore;
class ObserveStrategy;
struct SolutionInfo;
class StockStorageBase;
class StockSolutionAPI IntegratedStrategy : public Solution
{
public:
	/** 构造函数
	*/
	IntegratedStrategy();

public:
	/** 初始化
	@param [in] vecStrategyType 执行类型
	*/
	void init(const std::vector<ChooseParam>& vecChooseParam, StockStorageBase* storage);

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
	/** 设置每项选择
	@param [in] chooseParam 每项选择
	@param [in] date 当前日期
	*/
	void setEveryChooseParam(const ChooseParam& chooseParam, const IntDateTime& date);

protected:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::shared_ptr<Solution> m_useSolution;
	std::vector<ChooseParam> m_vecChooseParam;
	StockStorageBase* m_storage;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};