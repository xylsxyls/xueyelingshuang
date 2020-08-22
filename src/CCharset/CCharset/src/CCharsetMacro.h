#pragma once

#ifdef _MSC_VER
    #ifdef _CCharsetAPI
        #define CCharsetAPI _declspec(dllimport)
    #else
        #define CCharsetAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define CCharsetAPI __attribute__ ((visibility("default")))
    #else
        #define CCharsetAPI
    #endif
#endif

#if defined(STATIC_LIB)
    #undef CCharsetAPI
    #define CCharsetAPI 
#endif