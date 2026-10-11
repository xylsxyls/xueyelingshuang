#pragma once

#ifdef _MSC_VER
    #ifdef _KissFFTCppAPI
        #define KissFFTCppAPI _declspec(dllimport)
    #else
        #define KissFFTCppAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define KissFFTCppAPI __attribute__ ((visibility("default")))
    #else
        #define KissFFTCppAPI
    #endif
#endif

#if defined(STATIC_LIB)
    #undef KissFFTCppAPI
    #define KissFFTCppAPI
#endif