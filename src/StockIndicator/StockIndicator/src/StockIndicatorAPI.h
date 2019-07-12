#pragma once
#define _StockIndicatorAPI
#include "StockIndicator.h"
#include "StockWrIndicator.h"
#include "StockRsiIndicator.h"
#include "StockWr.h"
#include "StockRsi.h"

#ifdef _DEBUG
#pragma comment(lib,"StockIndicatord.lib")
#else
#pragma comment(lib,"StockIndicator.lib")
#endif