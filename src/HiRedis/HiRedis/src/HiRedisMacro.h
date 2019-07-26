#pragma once

#ifdef _HiRedisAPI
#define HiRedisAPI _declspec(dllimport)
#else
#define HiRedisAPI _declspec(dllexport)
#endif

#if defined(STATIC_LIB)
#undef HiRedisAPI
#define HiRedisAPI 
#endif