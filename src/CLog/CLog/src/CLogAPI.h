#pragma once
#define _CLogAPI
#include "CLog.h"

#ifdef _DEBUG
#pragma comment(lib,"CLogd.lib")
#else
#pragma comment(lib,"CLog.lib")
#endif