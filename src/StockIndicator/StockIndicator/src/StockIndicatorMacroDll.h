#pragma once

#ifdef _StockIndicatorAPI
#define StockIndicatorAPI _declspec(dllimport)
#else
#define StockIndicatorAPI _declspec(dllexport)
#endif