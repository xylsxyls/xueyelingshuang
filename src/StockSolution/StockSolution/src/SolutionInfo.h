#pragma once
#include "StockSolutionMacro.h"
#include <map>
#include <memory>
#include "StockStrategy/StockStrategyAPI.h"

struct StrategyInfo;
class StockFund;
struct StockSolutionAPI SolutionInfo
{
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	//执行的zhanghu
	StockFund* m_fund;
	//gupiaochi
	std::vector<std::string>* m_filterStock;
	//使用的类型，in,out
	StrategyType m_useType;
	//stock，策略类型
	std::map<std::string, std::map<StrategyType, std::pair<std::shared_ptr<StrategyInfo>, std::shared_ptr<StrategyInfo>>>> m_strategyAllInfo;
#ifdef _MSC_VER
#pragma warning(pop)
#endif

	/** 构造函数
	*/
	SolutionInfo();
	
	/** 析构函数
	*/
	virtual ~SolutionInfo();

public:
	/** 设置参数
	@param [in] useType 使用的类型
	*/
	void setParam(StrategyType useType);

	/** 获取执行的策略
	@param [in] stock gupiao代码
	@param [in] useType 使用的类型
	@return 返回执行的策略
	*/
	std::shared_ptr<StrategyInfo> strategyInfo(const std::string& stock, StrategyType useType);

	/** 获取执行计数的策略
	@param [in] stock gupiao代码
	@param [in] useType 使用的类型
	@return 返回执行计数的策略
	*/
	std::shared_ptr<StrategyInfo> strategyInfoCount(const std::string& stock, StrategyType useType);
};