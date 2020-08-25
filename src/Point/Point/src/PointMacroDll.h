#pragma once

#ifdef _MSC_VER
    #ifdef _PointAPI
        #define PointAPI _declspec(dllimport)
    #else
        #define PointAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define PointAPI __attribute__ ((visibility("default")))
    #else
        #define PointAPI
    #endif
#endif