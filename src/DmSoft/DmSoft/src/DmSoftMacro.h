#pragma once

#ifdef _MSC_VER
    #ifdef _DmSoftAPI
        #define DmSoftAPI _declspec(dllimport)
    #else
        #define DmSoftAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define DmSoftAPI __attribute__ ((visibility("default")))
    #else
        #define DmSoftAPI
    #endif
#endif

#if defined(STATIC_LIB)
    #undef DmSoftAPI
    #define DmSoftAPI 
#endif