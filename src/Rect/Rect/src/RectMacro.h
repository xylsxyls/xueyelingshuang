#pragma once

#ifdef _MSC_VER
    #ifdef _RectAPI
        #define RectAPI _declspec(dllimport)
    #else
        #define RectAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define RectAPI __attribute__ ((visibility("default")))
    #else
        #define RectAPI
    #endif
#endif

#if defined(STATIC_LIB)
    #undef RectAPI
    #define RectAPI 
#endif