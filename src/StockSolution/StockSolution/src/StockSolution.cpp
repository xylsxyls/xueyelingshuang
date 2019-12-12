#include "StockSolution.h"
#include "AvgFundHighScore.h"
#include "StrategySet.h"
#include "DisposableStrategy.h"
#include "AvgFundHighScoreInfo.h"
#include "StrategySetInfo.h"
#include "DisposableStrategyInfo.h"

StockSolution::StockSolution()
{

}

StockSolution& StockSolution::instance()
{
	static StockSolution s_stockSolution;
	return s_stockSolution;
}

std::shared_ptr<Solution> StockSolution::solution(SolutionType solutionType)
{
	std::shared_ptr<Solution> spSolution;
	switch (solutionType)
	{
	case AVG_FUND_HIGH_SCORE:
	{
		spSolution.reset(new AvgFundHighScore);
	}
	break;
	case STRATEGY_SET:
	{
		spSolution.reset(new StrategySet);
	}
	break;
	case DISPOSABLE_STRATEGY:
	{
		spSolution.reset(new DisposableStrategy);
	}
	break;
	default:
		break;
	}
	return spSolution;
}

std::shared_ptr<SolutionInfo> StockSolution::solutionInfo(SolutionType solutionType)
{
	std::shared_ptr<SolutionInfo> spSolutionInfo;
	switch (solutionType)
	{
	case AVG_FUND_HIGH_SCORE:
	{
		spSolutionInfo.reset(new AvgFundHighScoreInfo);
	}
	break;
	case STRATEGY_SET:
	{
		spSolutionInfo.reset(new StrategySetInfo);
	}
	break;
	case DISPOSABLE_STRATEGY:
	{
		spSolutionInfo.reset(new DisposableStrategyInfo);
	}
	break;
	default:
		break;
	}
	return spSolutionInfo;
}