#pragma once
#ifdef _WIN32
#define _CLoadDLLAPI
#endif
#include "CLoadDLL.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"CLoadDLLd.lib")
#else
#pragma comment(lib,"CLoadDLL.lib")
#endif
#endif