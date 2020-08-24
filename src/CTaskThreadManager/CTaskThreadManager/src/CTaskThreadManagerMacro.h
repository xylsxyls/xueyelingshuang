#pragma once

#ifdef _MSC_VER
    #ifdef _CTaskThreadManagerAPI
        #define CTaskThreadManagerAPI _declspec(dllimport)
    #else
        #define CTaskThreadManagerAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define CTaskThreadManagerAPI __attribute__ ((visibility("default")))
    #else
        #define CTaskThreadManagerAPI
    #endif
#endif

#if defined(STATIC_LIB)
    #undef CTaskThreadManagerAPI
    #define CTaskThreadManagerAPI 
#endif