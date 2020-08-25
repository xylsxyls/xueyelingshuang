#pragma once

#ifdef _MSC_VER
    #ifdef _RapidJsonAPI
        #define RapidJsonAPI _declspec(dllimport)
    #else
        #define RapidJsonAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define RapidJsonAPI __attribute__ ((visibility("default")))
    #else
        #define RapidJsonAPI
    #endif
#endif

#if defined(STATIC_LIB)
    #undef RapidJsonAPI
    #define RapidJsonAPI 
#endif