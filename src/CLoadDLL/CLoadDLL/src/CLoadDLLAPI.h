#pragma once
#define _CLoadDLLAPI
#include "CLoadDLL.h"

#ifdef NDEBUG
#pragma comment(lib,"CLoadDLL.lib")
#else
#pragma comment(lib,"CLoadDLLd.lib")
#endif