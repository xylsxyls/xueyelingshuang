#pragma once

#ifdef _MSC_VER
    #ifdef _CSystemAPI
        #define CSystemAPI _declspec(dllimport)
    #else
        #define CSystemAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define CSystemAPI __attribute__ ((visibility("default")))
    #else
        #define CSystemAPI
    #endif
#endif