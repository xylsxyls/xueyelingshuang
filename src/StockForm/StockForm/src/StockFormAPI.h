#pragma once
#ifdef _WIN32
#define _StockFormAPI
#endif
#include "StockForm.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"StockFormd.lib")
#else
#pragma comment(lib,"StockForm.lib")
#endif
#endif