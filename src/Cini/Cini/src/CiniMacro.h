#pragma once

#ifdef _MSC_VER
    #ifdef _CiniAPI
        #define CiniAPI _declspec(dllimport)
    #else
        #define CiniAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define CiniAPI __attribute__ ((visibility("default")))
    #else
        #define CiniAPI
    #endif
#endif

#if defined(STATIC_LIB)
    #undef CiniAPI
    #define CiniAPI 
#endif