#pragma once

#ifdef _MSC_VER
    #ifdef _NetChannelAPI
        #define NetChannelAPI _declspec(dllimport)
    #else
        #define NetChannelAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define NetChannelAPI __attribute__ ((visibility("default")))
    #else
        #define NetChannelAPI
    #endif
#endif