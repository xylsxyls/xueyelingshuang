#pragma once

#ifdef _MSC_VER
    #ifdef _CGetPathAPI
        #define CGetPathAPI _declspec(dllimport)
    #else
        #define CGetPathAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define CGetPathAPI __attribute__ ((visibility("default")))
    #else
        #define CGetPathAPI
    #endif
#endif