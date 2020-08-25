#pragma once

#ifdef _MSC_VER
    #ifdef _MxmlCppAPI
        #define MxmlCppAPI _declspec(dllimport)
    #else
        #define MxmlCppAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define MxmlCppAPI __attribute__ ((visibility("default")))
    #else
        #define MxmlCppAPI
    #endif
#endif

#if defined(STATIC_LIB)
    #undef MxmlCppAPI
    #define MxmlCppAPI 
#endif