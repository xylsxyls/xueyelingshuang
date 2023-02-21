#pragma once
#ifdef _WIN32
#define _CMouseConfigAPI
#endif
#include "CMouseConfig.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"CMouseConfigd.lib")
#else
#pragma comment(lib,"CMouseConfig.lib")
#endif
#endif