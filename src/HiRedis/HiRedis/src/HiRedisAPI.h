#pragma once
#define _HiRedisAPI
#include "HiRedis.h"
#include "HiRedisResultSet.h"

#ifdef _DEBUG
#pragma comment(lib,"HiRedisd.lib")
#else
#pragma comment(lib,"HiRedis.lib")
#endif