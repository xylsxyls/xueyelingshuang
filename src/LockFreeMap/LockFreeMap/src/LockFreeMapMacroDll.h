#pragma once

#ifdef _MSC_VER
    #ifdef _LockFreeMapAPI
        #define LockFreeMapAPI _declspec(dllimport)
    #else
        #define LockFreeMapAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define LockFreeMapAPI __attribute__ ((visibility("default")))
    #else
        #define LockFreeMapAPI
    #endif
#endif