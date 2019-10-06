#pragma once

#ifdef _StockFormAPI
#define StockFormAPI _declspec(dllimport)
#else
#define StockFormAPI _declspec(dllexport)
#endif

#if defined(STATIC_LIB)
#undef StockFormAPI
#define StockFormAPI 
#endif