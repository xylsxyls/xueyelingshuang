#pragma once

#ifdef _MSC_VER
    #ifdef _ThreadWindowAPI
        #define ThreadWindowAPI _declspec(dllimport)
    #else
        #define ThreadWindowAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define ThreadWindowAPI __attribute__ ((visibility("default")))
    #else
        #define ThreadWindowAPI
    #endif
#endif

#if defined(STATIC_LIB)
    #undef ThreadWindowAPI
    #define ThreadWindowAPI 
#endif