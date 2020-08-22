#pragma once

#ifdef _MSC_VER
    #ifdef _CStringManagerAPI
        #define CStringManagerAPI _declspec(dllimport)
    #else
        #define CStringManagerAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define CStringManagerAPI __attribute__ ((visibility("default")))
    #else
        #define CStringManagerAPI
    #endif
#endif