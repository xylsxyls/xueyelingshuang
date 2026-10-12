#pragma once

#ifdef _MSC_VER
    #ifdef _AccountAPI
        #define AccountAPI _declspec(dllimport)
    #else
        #define AccountAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define AccountAPI __attribute__ ((visibility("default")))
    #else
        #define AccountAPI
    #endif
#endif

#if defined(STATIC_LIB)
    #undef AccountAPI
    #define AccountAPI 
#endif