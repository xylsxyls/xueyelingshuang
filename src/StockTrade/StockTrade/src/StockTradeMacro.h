#pragma once

#ifdef _StockTradeAPI
#define StockTradeAPI _declspec(dllimport)
#else
#define StockTradeAPI _declspec(dllexport)
#endif

#if defined(STATIC_LIB)
#undef StockTradeAPI
#define StockTradeAPI 
#endif