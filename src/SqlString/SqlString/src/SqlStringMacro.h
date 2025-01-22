#pragma once

#ifdef _MSC_VER
    #ifdef _SqlStringAPI
        #define SqlStringAPI _declspec(dllimport)
    #else
        #define SqlStringAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define SqlStringAPI __attribute__ ((visibility("default")))
    #else
        #define SqlStringAPI
    #endif
#endif

#if defined(STATIC_LIB)
    #undef SqlStringAPI
    #define SqlStringAPI 
#endif