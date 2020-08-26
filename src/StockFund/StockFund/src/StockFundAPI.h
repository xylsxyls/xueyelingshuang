#pragma once
#ifdef _WIN32
#define _StockFundAPI
#endif
#include "StockFund.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"StockFundd.lib")
#else
#pragma comment(lib,"StockFund.lib")
#endif
#endif