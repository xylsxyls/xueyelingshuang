#pragma once

#ifdef _MSC_VER
    #ifdef _CScreenAPI
        #define CScreenAPI _declspec(dllimport)
    #else
        #define CScreenAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define CScreenAPI __attribute__ ((visibility("default")))
    #else
        #define CScreenAPI
    #endif
#endif