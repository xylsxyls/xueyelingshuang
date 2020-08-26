#pragma once

#ifdef _MSC_VER
    #ifdef _NetSenderAPI
        #define NetSenderAPI _declspec(dllimport)
    #else
        #define NetSenderAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define NetSenderAPI __attribute__ ((visibility("default")))
    #else
        #define NetSenderAPI
    #endif
#endif

#if defined(STATIC_LIB)
    #undef NetSenderAPI
    #define NetSenderAPI 
#endif