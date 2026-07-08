#include "QuantStrategyManager.h"
#include "S1100B1400Strategy.h"
#include "WaveStrategy.h"

QuantStrategyManager::QuantStrategyManager()
{

}

QuantStrategyManager& QuantStrategyManager::instance()
{
	static QuantStrategyManager manager;
	return manager;
}

std::shared_ptr<Strategy> QuantStrategyManager::createStrategy(StrategyMode mode)
{
	switch (mode)
	{
	case StrategyMode::S1100B1400:
		return std::make_shared<S1100B1400Strategy>();
	case StrategyMode::WAVE:
		return std::make_shared<WaveStrategy>();
	default:
		return nullptr;
	}
}