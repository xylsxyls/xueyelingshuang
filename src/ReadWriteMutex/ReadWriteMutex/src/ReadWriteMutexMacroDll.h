#pragma once

#ifdef _MSC_VER
    #ifdef _ReadWriteMutexAPI
        #define ReadWriteMutexAPI _declspec(dllimport)
    #else
        #define ReadWriteMutexAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define ReadWriteMutexAPI __attribute__ ((visibility("default")))
    #else
        #define ReadWriteMutexAPI
    #endif
#endif