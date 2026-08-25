#pragma once
#ifdef _WIN32
#define _LockFreeMapAPI
#endif
#include "LockFreeMap.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"LockFreeMapd.lib")
#else
#pragma comment(lib,"LockFreeMap.lib")
#endif
#endif