#pragma once

#ifdef _MSC_VER
    #ifdef _LogManagerAPI
        #define LogManagerAPI _declspec(dllimport)
    #else
        #define LogManagerAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define LogManagerAPI __attribute__ ((visibility("default")))
    #else
        #define LogManagerAPI
    #endif
#endif