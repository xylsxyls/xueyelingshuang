#pragma once

#ifdef _StockStorageAPI
#define StockStorageAPI _declspec(dllimport)
#else
#define StockStorageAPI _declspec(dllexport)
#endif