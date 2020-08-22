#pragma once

#ifdef _MSC_VER
    #ifdef _ColorAPI
        #define ColorAPI _declspec(dllimport)
    #else
        #define ColorAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define ColorAPI __attribute__ ((visibility("default")))
    #else
        #define ColorAPI
    #endif
#endif

#if defined(STATIC_LIB)
    #undef ColorAPI
    #define ColorAPI 
#endif