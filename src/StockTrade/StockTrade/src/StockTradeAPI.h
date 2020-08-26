#pragma once
#ifdef _WIN32
#define _StockTradeAPI
#endif
#include "StockTrade.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"StockTraded.lib")
#else
#pragma comment(lib,"StockTrade.lib")
#endif
#endif