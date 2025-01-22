#pragma once

#ifdef _MSC_VER
    #ifdef _ConfigManagerAPI
        #define ConfigManagerAPI _declspec(dllimport)
    #else
        #define ConfigManagerAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define ConfigManagerAPI __attribute__ ((visibility("default")))
    #else
        #define ConfigManagerAPI
    #endif
#endif