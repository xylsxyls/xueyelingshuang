#pragma once
#include "Solution.h"
#include <memory>

class Strategy;
struct SolutionAllInfo;
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
	@param [out] buyStock 选出的gupiao集合，stock,price,rate0-1
	@param [in] date 日期
	@param [in] solutionAllInfo 解决方案需要的信息
	@return 是否有选出的gupiao
	*/
	bool buy(std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>& buyStock,
		const IntDateTime& date,
		const std::shared_ptr<SolutionAllInfo>& solutionAllInfo);

	/** 询问单只gupiao需不需要maichu
	@param [in] sellStock maichu的gupiao集合，stock,price,rate0-1
	@param [in] date 日期
	@param [in] solutionAllInfo 解决方案需要的信息
	@return 返回是否有需要maichu的gupiao
	*/
	bool sell(std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>& sellStock,
		const IntDateTime& date,
		const std::shared_ptr<SolutionAllInfo>& solutionAllInfo);

protected:
	/** 根据策略选出可以goumai的gupiao
	@param [out] buyStock 选出的gupiao集合，stock,price,score0-100
	@param [in] date 日期
	@param [in] solutionAllInfo 解决方案需要的信息
	@return 是否有选出的gupiao
	*/
	bool strategyBuy(std::vector<std::pair<std::string, std::pair<BigNumber, std::pair<BigNumber, BigNumber>>>>& buyStock,
		const IntDateTime& date,
		const std::shared_ptr<SolutionAllInfo>& solutionAllInfo);

	/** 获取可mairu的计数
	@param[in] date 日期
	@param[in] solutionAllInfo 解决方案需要的信息
	@return 返回可mairu的计数
	*/
	int32_t strategyBuyCount(const IntDateTime& date, const std::shared_ptr<SolutionAllInfo>& solutionAllInfo);

	/** 询问单只gupiao需不需要maichu
	@param [in] sellStock maichu的gupiao集合，stock,price,score0-100
	@param [in] date 日期
	@param [in] solutionAllInfo 解决方案需要的信息
	@return 返回是否有需要maichu的gupiao
	*/
	bool strategySell(std::vector<std::pair<std::string, std::pair<BigNumber, std::pair<BigNumber, BigNumber>>>>& sellStock,
		const IntDateTime& date,
		const std::shared_ptr<SolutionAllInfo>& solutionAllInfo);

protected:
	int32_t m_stockNum;
	int32_t m_minPollSize;
};