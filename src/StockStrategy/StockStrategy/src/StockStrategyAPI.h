#pragma once
#define _StockStrategyAPI
#include "StockStrategy.h"
#include "Strategy.h"

#ifdef _DEBUG
#pragma comment(lib,"StockStrategyd.lib")
#else
#pragma comment(lib,"StockStrategy.lib")
#endif