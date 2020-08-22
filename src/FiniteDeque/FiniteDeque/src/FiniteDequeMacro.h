#pragma once

#ifdef _MSC_VER
    #ifdef _FiniteDequeAPI
        #define FiniteDequeAPI _declspec(dllimport)
    #else
        #define FiniteDequeAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define FiniteDequeAPI __attribute__ ((visibility("default")))
    #else
        #define FiniteDequeAPI
    #endif
#endif

#if defined(STATIC_LIB)
    #undef FiniteDequeAPI
    #define FiniteDequeAPI 
#endif