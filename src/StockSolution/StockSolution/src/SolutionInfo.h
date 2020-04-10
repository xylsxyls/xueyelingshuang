#pragma once
#include "StockSolutionMacro.h"
#include <map>
#include <memory>
#include "StockStrategy/StockStrategyAPI.h"
#include "StrategyStruct.h"
#include "ChooseParam.h"

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
	//选择参数，in,out
	ChooseParam m_chooseParam;
	//策略类型
	std::map<StrategyType, StrategyStruct> m_allStrategy;
	//stock，策略类型
	std::map<std::string, std::map<StrategyType, StrategyInfoStruct>> m_allStrategyInfo;
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
	/** 获取执行的策略
	@param [in] useType 使用的类型，如果是默认值表示使用类内类型
	@return 返回执行的策略
	*/
	std::shared_ptr<Strategy> strategy(StrategyType useType = STRATEGY_INIT);

	/** 获取执行计数的策略
	@param [in] useType 使用的类型，如果是默认值表示使用类内类型
	@return 返回执行计数的策略
	*/
	std::shared_ptr<Strategy> strategyCount(StrategyType useType = STRATEGY_INIT);

	/** 获取执行的策略信息
	@param [in] useType 使用的类型，如果是默认值表示使用类内类型
	@param [in] stock gupiao代码，如果是默认值表示使用第一个
	@return 返回执行的策略信息
	*/
	std::shared_ptr<StrategyInfo> strategyInfo(StrategyType useType = STRATEGY_INIT, const std::string& stock = "");

	/** 获取执行计数的策略信息
	@param [in] useType 使用的类型，如果是默认值表示使用类内类型
	@param [in] stock gupiao代码，如果是默认值表示使用第一个
	@return 返回执行计数的策略信息
	*/
	std::shared_ptr<StrategyInfo> strategyCountInfo(StrategyType useType = STRATEGY_INIT, const std::string& stock = "");
};