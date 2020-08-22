#pragma once

#ifdef _MSC_VER
    #ifdef _CLoadDLLAPI
        #define CLoadDLLAPI _declspec(dllimport)
    #else
        #define CLoadDLLAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define CLoadDLLAPI __attribute__ ((visibility("default")))
    #else
        #define CLoadDLLAPI
    #endif
#endif

#if defined(STATIC_LIB)
    #undef CLoadDLLAPI
    #define CLoadDLLAPI 
#endif