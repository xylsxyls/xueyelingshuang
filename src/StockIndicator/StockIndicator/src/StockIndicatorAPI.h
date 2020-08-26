#pragma once
#ifdef _WIN32
#define _StockIndicatorAPI
#endif
#include "StockIndicator.h"
#include "StockWrIndicator.h"
#include "StockRsiIndicator.h"
#include "StockSarIndicator.h"
#include "StockbollIndicator.h"
#include "StockAvgIndicator.h"
#include "StockWr.h"
#include "StockRsi.h"
#include "StockSar.h"
#include "StockBoll.h"
#include "StockAvg.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"StockIndicatord.lib")
#else
#pragma comment(lib,"StockIndicator.lib")
#endif
#endif