#pragma once

#ifdef _StockRetestAPI
#define StockRetestAPI _declspec(dllimport)
#else
#define StockRetestAPI _declspec(dllexport)
#endif

#if defined(STATIC_LIB)
#undef StockRetestAPI
#define StockRetestAPI 
#endif