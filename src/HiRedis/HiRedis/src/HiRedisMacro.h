#pragma once

#ifdef _MSC_VER
    #ifdef _HiRedisAPI
        #define HiRedisAPI _declspec(dllimport)
    #else
        #define HiRedisAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define HiRedisAPI __attribute__ ((visibility("default")))
    #else
        #define HiRedisAPI
    #endif
#endif

#if defined(STATIC_LIB)
    #undef HiRedisAPI
    #define HiRedisAPI 
#endif