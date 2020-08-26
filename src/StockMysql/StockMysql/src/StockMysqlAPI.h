#pragma once
#ifdef _WIN32
#define _StockMysqlAPI
#endif
#include "StockMysql.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"StockMysqld.lib")
#else
#pragma comment(lib,"StockMysql.lib")
#endif
#endif