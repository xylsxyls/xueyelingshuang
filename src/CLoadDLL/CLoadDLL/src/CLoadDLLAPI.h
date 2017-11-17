#pragma once
#define _CLoadDLLAPI
#include "CLoadDLL.h"

#ifdef _DEBUG
#pragma comment(lib,"CLoadDLLd.lib")
#else
#pragma comment(lib,"CLoadDLL.lib")
#endif