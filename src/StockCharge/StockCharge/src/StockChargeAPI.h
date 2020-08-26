#pragma once
#ifdef _WIN32
#define _StockChargeAPI
#endif
#include "StockCharge.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"StockCharged.lib")
#else
#pragma comment(lib,"StockCharge.lib")
#endif
#endif