#pragma once
#ifdef _WIN32
#define _StockRetestAPI
#endif
#include "StockEveryRetest.h"
#include "StockRealRetest.h"
#include "StockOnceRetest.h"
#include "StockDaysRetest.h"
#include "StockRankRetest.h"
#include "StockChanceRetest.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"StockRetestd.lib")
#else
#pragma comment(lib,"StockRetest.lib")
#endif
#endif