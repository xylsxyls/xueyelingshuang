#pragma once
#ifdef _WIN32
#define _TraceDataMapAPI
#endif
#include "TraceDataMap.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"TraceDataMapd.lib")
#else
#pragma comment(lib,"TraceDataMap.lib")
#endif
#endif