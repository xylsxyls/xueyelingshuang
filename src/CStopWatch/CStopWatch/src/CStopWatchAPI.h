#pragma once
#ifdef _WIN32
#define _CStopWatchAPI
#endif
#include "CStopWatch.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"CStopWatchd.lib")
#else
#pragma comment(lib,"CStopWatch.lib")
#endif
#endif