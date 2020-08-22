#pragma once

#ifdef _MSC_VER
    #ifdef _CHookAPI
        #define CHookAPI _declspec(dllimport)
    #else
        #define CHookAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define CHookAPI __attribute__ ((visibility("default")))
    #else
        #define CHookAPI
    #endif
#endif