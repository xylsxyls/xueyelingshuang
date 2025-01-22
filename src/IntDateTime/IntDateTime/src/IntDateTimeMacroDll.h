#pragma once

#ifdef _MSC_VER
    #ifdef _IntDateTimeAPI
        #define IntDateTimeAPI _declspec(dllimport)
    #else
        #define IntDateTimeAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define IntDateTimeAPI __attribute__ ((visibility("default")))
    #else
        #define IntDateTimeAPI
    #endif
#endif