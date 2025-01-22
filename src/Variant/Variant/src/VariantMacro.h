#pragma once

#ifdef _MSC_VER
    #ifdef _VariantAPI
        #define VariantAPI _declspec(dllimport)
    #else
        #define VariantAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define VariantAPI __attribute__ ((visibility("default")))
    #else
        #define VariantAPI
    #endif
#endif

#if defined(STATIC_LIB)
    #undef VariantAPI
    #define VariantAPI 
#endif