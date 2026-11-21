#pragma once

#ifdef _MSC_VER
    #ifdef _PlatformTestAPI
        #define PlatformTestAPI _declspec(dllimport)
    #else
        #define PlatformTestAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define PlatformTestAPI __attribute__ ((visibility("default")))
    #else
        #define PlatformTestAPI 
    #endif
#endif

#if defined(STATIC_LIB)
    #undef PlatformTestAPI
    #define PlatformTestAPI 
#endif