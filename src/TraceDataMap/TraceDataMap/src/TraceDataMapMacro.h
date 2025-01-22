#pragma once

#ifdef _MSC_VER
    #ifdef _TraceDataMapAPI
        #define TraceDataMapAPI _declspec(dllimport)
    #else
        #define TraceDataMapAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define TraceDataMapAPI __attribute__ ((visibility("default")))
    #else
        #define TraceDataMapAPI
    #endif
#endif

#if defined(STATIC_LIB)
    #undef TraceDataMapAPI
    #define TraceDataMapAPI 
#endif