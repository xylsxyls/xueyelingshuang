#pragma once
#define _StockMysqlAPI
#include "StockMysql.h"

#ifdef _DEBUG
#pragma comment(lib,"StockMysqld.lib")
#else
#pragma comment(lib,"StockMysql.lib")
#endif