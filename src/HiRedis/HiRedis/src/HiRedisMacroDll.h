#pragma once

#ifdef _HiRedisAPI
#define HiRedisAPI _declspec(dllimport)
#else
#define HiRedisAPI _declspec(dllexport)
#endif