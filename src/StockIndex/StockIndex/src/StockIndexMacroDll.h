#pragma once

#ifdef _StockIndexAPI
#define StockIndexAPI _declspec(dllimport)
#else
#define StockIndexAPI _declspec(dllexport)
#endif