#pragma once

#ifdef _MSC_VER
    #ifdef _LockFreeQueueAPI
        #define LockFreeQueueAPI _declspec(dllimport)
    #else
        #define LockFreeQueueAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define LockFreeQueueAPI __attribute__ ((visibility("default")))
    #else
        #define LockFreeQueueAPI
    #endif
#endif

#if defined(STATIC_LIB)
    #undef LockFreeQueueAPI
    #define LockFreeQueueAPI 
#endif