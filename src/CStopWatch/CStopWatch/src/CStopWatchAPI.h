#pragma once
#define _CStopWatchAPI
#include "CStopWatch.h"

#ifdef NDEBUG
#pragma comment(lib,"CStopWatch.lib")
#else
#pragma comment(lib,"CStopWatchd.lib")
#endif