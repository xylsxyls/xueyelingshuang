#pragma once

#ifdef _MSC_VER
    #ifdef _LockFreeHashMapAPI
        #define LockFreeHashMapAPI _declspec(dllimport)
    #else
        #define LockFreeHashMapAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define LockFreeHashMapAPI __attribute__ ((visibility("default")))
    #else
        #define LockFreeHashMapAPI
    #endif
#endif