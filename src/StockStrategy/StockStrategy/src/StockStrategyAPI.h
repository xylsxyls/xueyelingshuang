#pragma once
#define _StockStrategyAPI
#include "StockStrategy.h"
#include "SarRiseBack.h"
#include "SarRiseBackInfo.h"
#include "CatchUp.h"
#include "CatchUpInfo.h"
#include "SarRiseBackThirtyLine.h"

#ifdef _DEBUG
#pragma comment(lib,"StockStrategyd.lib")
#else
#pragma comment(lib,"StockStrategy.lib")
#endif