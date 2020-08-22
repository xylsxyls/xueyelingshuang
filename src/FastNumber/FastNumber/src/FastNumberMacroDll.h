#pragma once

#ifdef _MSC_VER
    #ifdef _FastNumberAPI
        #define FastNumberAPI _declspec(dllimport)
    #else
        #define FastNumberAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define FastNumberAPI __attribute__ ((visibility("default")))
    #else
        #define FastNumberAPI
    #endif
#endif