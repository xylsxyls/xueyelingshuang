#pragma once
#ifdef _WIN32
#define _HiRedisAPI
#endif
#include "HiRedis.h"
#include "HiRedisResultSet.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"HiRedisd.lib")
#else
#pragma comment(lib,"HiRedis.lib")
#endif
#endif