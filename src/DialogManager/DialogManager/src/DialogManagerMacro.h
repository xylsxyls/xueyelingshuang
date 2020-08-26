#pragma once

#ifdef _MSC_VER
    #ifdef _DialogManagerAPI
        #define DialogManagerAPI _declspec(dllimport)
    #else
        #define DialogManagerAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define DialogManagerAPI __attribute__ ((visibility("default")))
    #else
        #define DialogManagerAPI
    #endif
#endif

#if defined(STATIC_LIB)
    #undef DialogManagerAPI
    #define DialogManagerAPI 
#endif