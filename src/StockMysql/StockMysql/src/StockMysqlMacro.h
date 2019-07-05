#pragma once

#ifdef _StockMysqlAPI
#define StockMysqlAPI _declspec(dllimport)
#else
#define StockMysqlAPI _declspec(dllexport)
#endif

#if defined(STATIC_LIB)
#undef StockMysqlAPI
#define StockMysqlAPI 
#endif