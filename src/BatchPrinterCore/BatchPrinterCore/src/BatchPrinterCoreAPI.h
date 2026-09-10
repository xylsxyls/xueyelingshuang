#pragma once
#ifdef _WIN32
#define _BatchPrinterCoreAPI
#endif
#include "BatchPrinterCore.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"BatchPrinterCored.lib")
#else
#pragma comment(lib,"BatchPrinterCore.lib")
#endif
#endif