#pragma once
#define _StockRetestAPI
#include "StockEveryRetest.h"
#include "StockRealRetest.h"
#include "StockOnceRetest.h"

#ifdef _DEBUG
#pragma comment(lib,"StockRetestd.lib")
#else
#pragma comment(lib,"StockRetest.lib")
#endif