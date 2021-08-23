#pragma once

#ifdef _MSC_VER
    #ifdef _WebSocketWorkAPI
        #define WebSocketWorkAPI _declspec(dllimport)
    #else
        #define WebSocketWorkAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define WebSocketWorkAPI __attribute__ ((visibility("default")))
    #else
        #define WebSocketWorkAPI
    #endif
#endif

#if defined(STATIC_LIB)
    #undef WebSocketWorkAPI
    #define WebSocketWorkAPI 
#endif