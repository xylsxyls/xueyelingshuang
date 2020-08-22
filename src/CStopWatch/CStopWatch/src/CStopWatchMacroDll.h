#pragma once

#ifdef _MSC_VER
    #ifdef _CStopWatchAPI
        #define CStopWatchAPI _declspec(dllimport)
    #else
        #define CStopWatchAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define CStopWatchAPI __attribute__ ((visibility("default")))
    #else
        #define CStopWatchAPI
    #endif
#endif