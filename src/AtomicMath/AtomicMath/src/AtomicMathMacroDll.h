#pragma once

#ifdef _MSC_VER
    #ifdef _AtomicMathAPI
        #define AtomicMathAPI _declspec(dllimport)
    #else
        #define AtomicMathAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define AtomicMathAPI __attribute__ ((visibility("default")))
    #else
        #define AtomicMathAPI
    #endif
#endif