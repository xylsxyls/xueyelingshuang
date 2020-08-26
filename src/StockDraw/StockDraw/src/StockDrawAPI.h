#pragma once
#ifdef _WIN32
#define _StockDrawAPI
#endif
#include "StockDraw.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"StockDrawd.lib")
#else
#pragma comment(lib,"StockDraw.lib")
#endif
#endif