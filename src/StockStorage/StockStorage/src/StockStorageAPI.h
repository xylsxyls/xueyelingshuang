#pragma once
#define _StockStorageAPI
#include "StockStorage.h"

#ifdef _DEBUG
#pragma comment(lib,"StockStoraged.lib")
#else
#pragma comment(lib,"StockStorage.lib")
#endif