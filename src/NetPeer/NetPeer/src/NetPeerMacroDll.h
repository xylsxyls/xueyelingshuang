#pragma once

#ifdef _MSC_VER
    #ifdef _NetPeerAPI
        #define NetPeerAPI _declspec(dllimport)
    #else
        #define NetPeerAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define NetPeerAPI __attribute__ ((visibility("default")))
    #else
        #define NetPeerAPI
    #endif
#endif