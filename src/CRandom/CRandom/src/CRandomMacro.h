#pragma once

#ifdef _MSC_VER
    #ifdef _CRandomAPI
        #define CRandomAPI _declspec(dllimport)
    #else
        #define CRandomAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define CRandomAPI __attribute__ ((visibility("default")))
    #else
        #define CRandomAPI
    #endif
#endif

#if defined(STATIC_LIB)
    #undef CRandomAPI
    #define CRandomAPI 
#endif