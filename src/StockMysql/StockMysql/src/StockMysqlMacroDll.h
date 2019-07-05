#pragma once

#ifdef _StockMysqlAPI
#define StockMysqlAPI _declspec(dllimport)
#else
#define StockMysqlAPI _declspec(dllexport)
#endif