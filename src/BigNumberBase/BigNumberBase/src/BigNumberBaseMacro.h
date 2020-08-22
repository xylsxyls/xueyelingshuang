#pragma once

#ifdef _MSC_VER
    #ifdef _BigNumberBaseAPI
        #define BigNumberBaseAPI _declspec(dllimport)
    #else
        #define BigNumberBaseAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define BigNumberBaseAPI __attribute__ ((visibility("default")))
    #else
        #define BigNumberBaseAPI
    #endif
#endif

#if defined(STATIC_LIB)
    #undef BigNumberBaseAPI
    #define BigNumberBaseAPI 
#endif