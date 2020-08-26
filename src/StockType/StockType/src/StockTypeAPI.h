#pragma once
#ifdef _WIN32
#define _StockTypeAPI
#endif
#include "StockType.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"StockTyped.lib")
#else
#pragma comment(lib,"StockType.lib")
#endif
#endif