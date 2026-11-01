#pragma once

#ifdef _MSC_VER
    #ifdef NET_CHANNEL_BUILD
        #define NetChannelAPI _declspec(dllexport)
    #else
        #define NetChannelAPI _declspec(dllimport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define NetChannelAPI __attribute__ ((visibility("default")))
    #else
        #define NetChannelAPI
    #endif
#endif

#if defined(STATIC_LIB)
    #undef NetChannelAPI
    #define NetChannelAPI
#endif