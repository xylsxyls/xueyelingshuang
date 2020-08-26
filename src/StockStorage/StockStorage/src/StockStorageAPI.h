#pragma once
#ifdef _WIN32
#define _StockStorageAPI
#endif
#include "StockStorage.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"StockStoraged.lib")
#else
#pragma comment(lib,"StockStorage.lib")
#endif
#endif