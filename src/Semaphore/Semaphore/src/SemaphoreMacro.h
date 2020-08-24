#pragma once

#ifdef _MSC_VER
    #ifdef _SemaphoreAPI
        #define SemaphoreAPI _declspec(dllimport)
    #else
        #define SemaphoreAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define SemaphoreAPI __attribute__ ((visibility("default")))
    #else
        #define SemaphoreAPI
    #endif
#endif

#if defined(STATIC_LIB)
    #undef SemaphoreAPI
    #define SemaphoreAPI 
#endif