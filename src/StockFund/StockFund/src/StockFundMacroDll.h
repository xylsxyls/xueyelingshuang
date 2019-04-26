#pragma once

#ifdef _StockFundAPI
#define StockFundAPI _declspec(dllimport)
#else
#define StockFundAPI _declspec(dllexport)
#endif