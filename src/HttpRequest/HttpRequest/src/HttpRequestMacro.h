#pragma once

#ifdef _MSC_VER
    #ifdef _HttpRequestAPI
        #define HttpRequestAPI _declspec(dllimport)
    #else
        #define HttpRequestAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define HttpRequestAPI __attribute__ ((visibility("default")))
    #else
        #define HttpRequestAPI
    #endif
#endif

#if defined(STATIC_LIB)
    #undef HttpRequestAPI
    #define HttpRequestAPI 
#endif