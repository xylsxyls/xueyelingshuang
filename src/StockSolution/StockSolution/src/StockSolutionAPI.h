#pragma once
#define _StockSolutionAPI
#include "StockSolution.h"
#include "AvgFundHighScore.h"
#include "AvgFundHighScoreInfo.h"
#include "StrategySet.h"
#include "StrategySetInfo.h"
#include "DisposableStrategy.h"
#include "DisposableStrategyInfo.h"
#include "IntegratedStrategy.h"
#include "IntegratedStrategyInfo.h"
#include "ObserveStrategy.h"
#include "ObserveStrategyInfo.h"

#ifdef _DEBUG
#pragma comment(lib,"StockSolutiond.lib")
#else
#pragma comment(lib,"StockSolution.lib")
#endif