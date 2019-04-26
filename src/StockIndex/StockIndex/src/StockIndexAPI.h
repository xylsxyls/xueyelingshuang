#pragma once
#define _StockIndexAPI
#include "StockIndex.h"

#ifdef _DEBUG
#pragma comment(lib,"StockIndexd.lib")
#else
#pragma comment(lib,"StockIndex.lib")
#endif