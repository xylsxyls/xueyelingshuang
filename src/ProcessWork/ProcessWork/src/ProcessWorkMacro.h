#pragma once

#ifdef _MSC_VER
    #ifdef _ProcessWorkAPI
        #define ProcessWorkAPI _declspec(dllimport)
    #else
        #define ProcessWorkAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define ProcessWorkAPI __attribute__ ((visibility("default")))
    #else
        #define ProcessWorkAPI
    #endif
#endif

#if defined(STATIC_LIB)
    #undef ProcessWorkAPI
    #define ProcessWorkAPI 
#endif