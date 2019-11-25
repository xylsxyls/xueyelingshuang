#pragma once
#include "StockSolutionMacro.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include "BigNumber/BigNumberAPI.h"
#include <map>
#include <memory>
#include "StockSolutionMacro.h"

struct SolutionInfo;
struct SolutionAllInfo;
class Strategy;
class StockSolutionAPI Solution
{
public:
	/** 设置策略
	@param [in] spStrategy 策略
	*/
	void init(const std::shared_ptr<Strategy>& spStrategy);

	/** 选出可以goumai的gupiao
	@param [out] buyStock 选出的gupiao集合，stock,price,rate
	@param [in] date 日期
	@param [in] allBuyInfo 所有的已goumai信息
	@return 是否有选出的gupiao
	*/
	virtual bool buy(std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>& buyStock,
		const IntDateTime& date,
		const std::shared_ptr<SolutionAllInfo>& allSolutionInfo) = 0;

	/** 询问单只gupiao需不需要maichu
	@param [in] stock gupiao代码
	@param [in] date 日期
	@param [out] price maichu价格
	@param [out] percent maichu比例，0-100
	@param [in] buyInfo goumai信息
	@return 返回询问的gupiao需不需要maichu
	*/
	virtual bool sell(const IntDateTime& date,
		BigNumber& price,
		BigNumber& rate,
		const std::shared_ptr<SolutionInfo>& solutionInfo) = 0;

protected:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::shared_ptr<Strategy> m_spStrategy;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};