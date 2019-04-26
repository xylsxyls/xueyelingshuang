#pragma once

#ifdef _StockMarketAPI
#define StockMarketAPI _declspec(dllimport)
#else
#define StockMarketAPI _declspec(dllexport)
#endif