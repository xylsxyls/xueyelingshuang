#pragma once

#ifdef _StockRetestAPI
#define StockRetestAPI _declspec(dllimport)
#else
#define StockRetestAPI _declspec(dllexport)
#endif