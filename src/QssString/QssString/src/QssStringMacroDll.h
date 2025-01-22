#pragma once

#ifdef _MSC_VER
    #ifdef _QssStringAPI
        #define QssStringAPI _declspec(dllimport)
    #else
        #define QssStringAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define QssStringAPI __attribute__ ((visibility("default")))
    #else
        #define QssStringAPI
    #endif
#endif