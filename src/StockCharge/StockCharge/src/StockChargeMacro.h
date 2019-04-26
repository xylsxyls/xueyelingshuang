#pragma once

#ifdef _StockChargeAPI
#define StockChargeAPI _declspec(dllimport)
#else
#define StockChargeAPI _declspec(dllexport)
#endif

#if defined(STATIC_LIB)
#undef StockChargeAPI
#define StockChargeAPI 
#endif