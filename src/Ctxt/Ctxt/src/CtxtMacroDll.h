#pragma once

#ifdef _MSC_VER
    #ifdef _CtxtAPI
        #define CtxtAPI _declspec(dllimport)
    #else
        #define CtxtAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define CtxtAPI __attribute__ ((visibility("default")))
    #else
        #define CtxtAPI
    #endif
#endif