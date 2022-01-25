#pragma once

#ifdef _MSC_VER
    #ifdef _DirectShowAPI
        #define DirectShowAPI _declspec(dllimport)
    #else
        #define DirectShowAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define DirectShowAPI __attribute__ ((visibility("default")))
    #else
        #define DirectShowAPI
    #endif
#endif