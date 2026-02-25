#include "ScoreManager.h"

ScoreManager::ScoreManager()
{

}

ScoreManager& ScoreManager::instance()
{
	static ScoreManager s_scoreManager;
	return s_scoreManager;
}

std::pair<StrategyMode, std::vector<int32_t>> ScoreManager::bestStrategy(uint32_t time)
{
	std::vector<int32_t> param =
	{
		(int32_t)ObserveTime::TIME1040,
		(int32_t)ObserveTime::TIME1100,
		(int32_t)ObserveTime::TIME1350,
		(int32_t)ObserveTime::TIME1400,
		(int32_t)ObserveTime::TIME1100,
		(int32_t)ObserveTime::TIME1350,
		0,
		-2,
		7,
		7,
		1
	};
	return std::pair<StrategyMode, std::vector<int32_t>>(StrategyMode::WAVE, param);
}