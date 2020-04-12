#pragma once
#include "StockSolutionMacro.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include "BigNumber/BigNumberAPI.h"
#include <map>
#include <memory>
#include "StockStrategy/StockStrategyAPI.h"
#include "ChooseParam.h"

struct SolutionInfo;
class StockSolutionAPI Solution
{
public:
	/** 设置参数
	@param [in] solutionInfo 解决方案信息
	*/
	void setSolutionInfo(const std::shared_ptr<SolutionInfo>& solutionInfo);

	/** 设置选择参数，必须在设置solutionInfo之后
	@param [in] chooseParam 选择参数
	*/
	void setChooseParam(const ChooseParam& chooseParam);

	/** 获取选择参数
	@return 返回选择参数
	*/
	ChooseParam chooseParam();

	/** 设置gupiaochi
	@param [in] filterStock gupiaochi
	*/
	void setFilterStock(std::vector<std::string>* filterStock);

	/** 设置执行的zhanghu
	@param [in] stockFund 执行的zhanghu
	*/
	void setStockFund(StockFund* stockFund);

	/** 选出可以goumai的gupiao
	@param [out] buyStock 选出的gupiao集合，stock,price,rate0-1
	@param [in] date 日期
	@return 是否有选出的gupiao
	*/
	virtual bool buy(std::vector<std::pair<std::string, StockInfo>>& buyStock, const IntDateTime& date);

	/** 询问单只gupiao需不需要maichu
	@param [in] sellStock maichu的gupiao集合，stock,price,rate0-1
	@param [in] date 日期
	@return 返回是否有需要maichu的gupiao
	*/
	virtual bool sell(std::vector<std::pair<std::string, StockInfo>>& sellStock, const IntDateTime& date);

	/** 获取解决方案类型
	@return 返回解决方案类型
	*/
	SolutionType type();

protected:
	SolutionType m_solutionType;

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::shared_ptr<SolutionInfo> m_solutionInfo;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	
};