#pragma once

#ifdef _StockDrawAPI
#define StockDrawAPI _declspec(dllimport)
#else
#define StockDrawAPI _declspec(dllexport)
#endif