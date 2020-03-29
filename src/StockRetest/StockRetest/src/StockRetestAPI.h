#pragma once
#define _StockRetestAPI
#include "StockEveryRetest.h"
#include "StockRealRetest.h"
#include "StockOnceRetest.h"
#include "StockDaysRetest.h"
#include "StockRankRetest.h"
#include "StockChanceRetest.h"

#ifdef _DEBUG
#pragma comment(lib,"StockRetestd.lib")
#else
#pragma comment(lib,"StockRetest.lib")
#endif