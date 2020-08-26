#pragma once

#ifdef _MSC_VER
    #ifdef _ManagerBaseAPI
        #define ManagerBaseAPI _declspec(dllimport)
    #else
        #define ManagerBaseAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define ManagerBaseAPI __attribute__ ((visibility("default")))
    #else
        #define ManagerBaseAPI
    #endif
#endif

#if defined(STATIC_LIB)
    #undef ManagerBaseAPI
    #define ManagerBaseAPI 
#endif