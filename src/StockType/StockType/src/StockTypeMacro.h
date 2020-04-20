#pragma once

#ifdef _StockTypeAPI
#define StockTypeAPI _declspec(dllimport)
#else
#define StockTypeAPI _declspec(dllexport)
#endif

#if defined(STATIC_LIB)
#undef StockTypeAPI
#define StockTypeAPI 
#endif