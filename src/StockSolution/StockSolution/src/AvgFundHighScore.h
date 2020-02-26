#pragma once
#include "Solution.h"
#include <memory>

class Strategy;
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
	*/
	void init(int32_t stockNum);

	/** 选出可以goumai的gupiao
	@param [out] buyStock 选出的gupiao集合，stock,price,rate0-1
	@param [in] date 日期
	@param [in] solutionInfo 解决方案需要的信息
	@return 是否有选出的gupiao
	*/
	virtual bool buy(std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>& buyStock,
		const IntDateTime& date,
		const std::shared_ptr<SolutionInfo>& solutionInfo);

	/** 询问单只gupiao需不需要maichu
	@param [in] sellStock maichu的gupiao集合，stock,price,rate0-1
	@param [in] date 日期
	@param [in] solutionInfo 解决方案需要的信息
	@return 返回是否有需要maichu的gupiao
	*/
	virtual bool sell(std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>& sellStock,
		const IntDateTime& date,
		const std::shared_ptr<SolutionInfo>& solutionInfo);

protected:
	/** 根据策略选出可以goumai的gupiao
	@param [out] buyStock 选出的gupiao集合，stock,price,score0-100
	@param [in] date 日期
	@param [in] solutionInfo 解决方案需要的信息
	@return 是否有选出的gupiao
	*/
	bool strategyBuy(std::vector<std::pair<std::string, std::pair<BigNumber, std::pair<BigNumber, BigNumber>>>>& buyStock,
		const IntDateTime& date,
		const std::shared_ptr<SolutionInfo>& solutionInfo);

	/** 获取可mairu的计数
	@param[in] date 日期
	@param[in] solutionInfo 解决方案需要的信息
	@return 返回可mairu的计数
	*/
	int32_t strategyBuyCount(const IntDateTime& date, const std::shared_ptr<SolutionInfo>& solutionInfo);

	/** 询问单只gupiao需不需要maichu
	@param [in] sellStock maichu的gupiao集合，stock,price,score0-100
	@param [in] date 日期
	@param [in] solutionInfo 解决方案需要的信息
	@return 返回是否有需要maichu的gupiao
	*/
	bool strategySell(std::vector<std::pair<std::string, std::pair<BigNumber, std::pair<BigNumber, BigNumber>>>>& sellStock,
		const IntDateTime& date,
		const std::shared_ptr<SolutionInfo>& solutionInfo);

	/** 获取最小通过个数
	@param [in] solutionInfo 解决方案需要的信息
	@return 返回最小通过个数
	*/
	int32_t minPollSize(const std::shared_ptr<SolutionInfo>& solutionInfo);

	/** 获取去掉个数
	@param [in] solutionInfo 解决方案需要的信息
	@return 返回去掉个数
	*/
	int32_t popSize(int32_t buySize, const std::shared_ptr<SolutionInfo>& solutionInfo);

protected:
	int32_t m_stockNum;
	int32_t m_minPollSize;
};