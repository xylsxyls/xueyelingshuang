#pragma once
#ifdef _WIN32
#define _StockStrategyAPI
#endif
#include "StockStrategy.h"
#include "SarRiseBack.h"
#include "SarRiseBackInfo.h"
#include "CatchUp.h"
#include "CatchUpInfo.h"
#include "SarRiseBackThirtyLine.h"
#include "SarRiseBackThirtyLineInfo.h"
#include "LineBack.h"
#include "LineBackInfo.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"StockStrategyd.lib")
#else
#pragma comment(lib,"StockStrategy.lib")
#endif
#endif