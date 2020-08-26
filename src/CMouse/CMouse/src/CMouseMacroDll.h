#pragma once

#ifdef _MSC_VER
    #ifdef _CMouseAPI
        #define CMouseAPI _declspec(dllimport)
    #else
        #define CMouseAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define CMouseAPI __attribute__ ((visibility("default")))
    #else
        #define CMouseAPI
    #endif
#endif