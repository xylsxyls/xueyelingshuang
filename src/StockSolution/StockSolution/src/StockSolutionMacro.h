#pragma once

#ifdef _StockSolutionAPI
#define StockSolutionAPI _declspec(dllimport)
#else
#define StockSolutionAPI _declspec(dllexport)
#endif

#if defined(STATIC_LIB)
#undef StockSolutionAPI
#define StockSolutionAPI 
#endif