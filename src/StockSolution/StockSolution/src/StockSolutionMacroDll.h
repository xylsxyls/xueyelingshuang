#pragma once

#ifdef _StockSolutionAPI
#define StockSolutionAPI _declspec(dllimport)
#else
#define StockSolutionAPI _declspec(dllexport)
#endif