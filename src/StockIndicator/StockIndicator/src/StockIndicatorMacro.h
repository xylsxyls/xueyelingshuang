#pragma once

#ifdef _StockIndicatorAPI
#define StockIndicatorAPI _declspec(dllimport)
#else
#define StockIndicatorAPI _declspec(dllexport)
#endif

#if defined(STATIC_LIB)
#undef StockIndicatorAPI
#define StockIndicatorAPI 
#endif