#pragma once
#include "Solution.h"
#include <memory>

class Strategy;
struct SolutionAllInfo;
struct SolutionInfo;
class StockSolutionAPI AvgFundHighScore : public Solution
{
public:
	/** 构造函数
	*/
	AvgFundHighScore();

public:
	/** 初始化
	@param [in] stockNum gupiao个数
	@param [in] minPollSize 最小允许通过的gupiaochi个数
	*/
	void init(int32_t stockNum, int32_t minPollSize);

	/** 选出可以goumai的gupiao
	@param [out] buyStock 选出的gupiao集合，stock,price,rate
	@param [in] date 日期
	@param [in] solutionAllInfo 解决方案需要的信息
	@return 是否有选出的gupiao
	*/
	bool buy(std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>& buyStock,
		const IntDateTime& date,
		const std::shared_ptr<SolutionAllInfo>& solutionAllInfo);

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
		const std::shared_ptr<SolutionInfo>& solutionInfo,
		const std::shared_ptr<SolutionAllInfo>& solutionAllInfo);

protected:
	/** 根据策略选出可以goumai的gupiao
	@param [out] buyStock 选出的gupiao集合，stock,price,rate
	@param [in] date 日期
	@param [in] solutionAllInfo 解决方案需要的信息
	@return 是否有选出的gupiao
	*/
	bool strategyBuy(std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>& buyStock,
		const IntDateTime& date,
		const std::shared_ptr<SolutionAllInfo>& solutionAllInfo);

protected:
	int32_t m_stockNum;
	int32_t m_minPollSize;
	int32_t m_sellNum;
};