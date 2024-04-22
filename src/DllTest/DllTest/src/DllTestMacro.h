#pragma once

#ifdef _MSC_VER
    #ifdef _DllTestAPI
        #define DllTestAPI _declspec(dllimport)
    #else
        #define DllTestAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define DllTestAPI __attribute__ ((visibility("default")))
    #else
        #define DllTestAPI
    #endif
#endif

#if defined(STATIC_LIB)
    #undef DllTestAPI
    #define DllTestAPI 
#endif