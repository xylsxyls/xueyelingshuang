#pragma once
#ifdef _WIN32
#define _StockMarketAPI
#endif
#include "StockMarket.h"
#include "StockDay.h"
#include "StockMarketHelper.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"StockMarketd.lib")
#else
#pragma comment(lib,"StockMarket.lib")
#endif
#endif