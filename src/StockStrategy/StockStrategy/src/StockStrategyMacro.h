#pragma once

#ifdef _StockStrategyAPI
#define StockStrategyAPI _declspec(dllimport)
#else
#define StockStrategyAPI _declspec(dllexport)
#endif

#if defined(STATIC_LIB)
#undef StockStrategyAPI
#define StockStrategyAPI 
#endif