#pragma once

#ifdef _MSC_VER
    #ifdef _NetWorkAPI
        #define NetWorkAPI _declspec(dllimport)
    #else
        #define NetWorkAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define NetWorkAPI __attribute__ ((visibility("default")))
    #else
        #define NetWorkAPI
    #endif
#endif

#if defined(STATIC_LIB)
    #undef NetWorkAPI
    #define NetWorkAPI 
#endif