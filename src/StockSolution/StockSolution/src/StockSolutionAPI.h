#pragma once
#ifdef _WIN32
#define _StockSolutionAPI
#endif
#include "StockSolution.h"
#include "AvgFundHighScore.h"
#include "StrategySet.h"
#include "DisposableStrategy.h"
#include "DisposableStrategyInfo.h"
#include "IntegratedStrategy.h"
#include "ObserveStrategy.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"StockSolutiond.lib")
#else
#pragma comment(lib,"StockSolution.lib")
#endif
#endif