#pragma once
#include "Solution.h"
#include <memory>

class Strategy;
struct SolutionAllInfo;
struct SolutionInfo;
class StockSolutionAPI AvgFundHighScore : public Solution
{
public:
	/** 选出可以goumai的gupiao
	@param [out] buyStock 选出的gupiao集合，stock,price,rate
	@param [in] date 日期
	@param [in] allBuyInfo 所有的已goumai信息
	@return 是否有选出的gupiao
	*/
	bool buy(std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>& buyStock,
		const IntDateTime& date,
		const std::shared_ptr<SolutionAllInfo>& allSolutionInfo);

	/** 询问单只gupiao需不需要maichu
	@param [in] stock gupiao代码
	@param [in] date 日期
	@param [out] price maichu价格
	@param [out] rate maichu比例，0-1
	@param [in] buyInfo goumai信息
	@return 返回询问的gupiao需不需要maichu
	*/
	bool sell(const IntDateTime& date,
		BigNumber& price,
		BigNumber& rate,
		const std::shared_ptr<SolutionInfo>& solutionInfo);
};