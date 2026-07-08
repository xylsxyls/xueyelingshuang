#pragma once
#include "Strategy.h"

class ScoreManager
{
private:
	ScoreManager();

public:
	static ScoreManager& instance();

public:
	/** 获取最佳策略
	@param [in] time 时间点
	@return 最佳策略类型，最佳策略参数
	*/
	std::pair<StrategyMode, std::vector<int32_t>> bestStrategy(uint32_t time);

private:
	/** 年化T收益率差值
	@param [in, out] vecStrategyResult 策略组，pair中第二个为分数，内部vector是不同周期的策略，外层vector是所有策略
	@param [in] highScore 该项最高分值
	*/
	void annualTDiff(std::vector<std::pair<std::vector<std::shared_ptr<StrategyResult>>, double>>& vecStrategyResult,
		double highScore);

	/** 年化T收益率总和
	@param [in, out] vecStrategyResult 策略组
	@param [in] count 计算的周期次数，从后向前
	@param [in] highScore 该项最高分值
	*/
	void annualT(std::vector<std::pair<std::vector<std::shared_ptr<StrategyResult>>, double>>& vecStrategyResult,
		uint32_t count, double highScore);

	/** 异常交易
	@param [in, out] vecStrategyResult 策略组
	@param [in] count 计算的周期次数，从前向后
	@param [in] highScore 该项最高分值
	*/
	void abnormal(std::vector<std::pair<std::vector<std::shared_ptr<StrategyResult>>, double>>& vecStrategyResult,
		uint32_t count, double highScore);

	/** 降序排序
	@param [in, out] vecStrategyResult 策略组
	*/
	void desc(std::vector<std::pair<std::vector<std::shared_ptr<StrategyResult>>, double>>& vecStrategyResult);

	/** 打印
	@param [in] vecStrategyResult 策略组
	*/
	void print(std::vector<std::pair<std::vector<std::shared_ptr<StrategyResult>>, double>>& vecStrategyResult);

	/** 百分比对应分数，越小越好
	@param [in] percentage 百分比指标
	@return 评分
	*/
	double smallBestScore(const BigNumber& percentage);

	/** 百分比对应分数，越大越好
	@param [in] percentage 百分比指标
	@return 评分
	*/
	double bigBestScore(const BigNumber& percentage);
};