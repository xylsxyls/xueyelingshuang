#pragma once
#define _CStopWatchAPI
#include "CStopWatch.h"

#ifdef _DEBUG
#pragma comment(lib,"CStopWatchd.lib")
#else
#pragma comment(lib,"CStopWatch.lib")
#endif