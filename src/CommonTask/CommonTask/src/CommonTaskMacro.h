#pragma once

#ifdef _MSC_VER
    #ifdef _CommonTaskAPI
        #define CommonTaskAPI _declspec(dllimport)
    #else
        #define CommonTaskAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define CommonTaskAPI __attribute__ ((visibility("default")))
    #else
        #define CommonTaskAPI
    #endif
#endif

#if defined(STATIC_LIB)
    #undef CommonTaskAPI
    #define CommonTaskAPI 
#endif