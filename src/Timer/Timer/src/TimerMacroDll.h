#pragma once

#ifdef _MSC_VER
    #ifdef _TimerAPI
        #define TimerAPI _declspec(dllimport)
    #else
        #define TimerAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define TimerAPI __attribute__ ((visibility("default")))
    #else
        #define TimerAPI
    #endif
#endif