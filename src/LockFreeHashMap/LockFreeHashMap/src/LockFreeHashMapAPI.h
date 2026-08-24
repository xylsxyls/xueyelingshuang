#pragma once
#ifdef _WIN32
#define _LockFreeHashMapAPI
#endif
#include "LockFreeHashMap.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"LockFreeHashMapd.lib")
#else
#pragma comment(lib,"LockFreeHashMap.lib")
#endif
#endif