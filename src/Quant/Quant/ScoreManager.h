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
};