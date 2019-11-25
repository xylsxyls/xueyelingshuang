#include "StockSolution.h"
#include "AvgFundHighScore.h"

StockSolution::StockSolution()
{

}

StockSolution& StockSolution::instance()
{
	static StockSolution s_stockSolution;
	return s_stockSolution;
}

std::shared_ptr<Solution> StockSolution::solution(SolutionEnum strategyEnum)
{
	std::shared_ptr<Solution> spSolution;
	switch (strategyEnum)
	{
	case AVG_FUND_HIGH_SCORE:
	{
		spSolution.reset(new AvgFundHighScore);
	}
	break;
	default:
		break;
	}
	return spSolution;
}