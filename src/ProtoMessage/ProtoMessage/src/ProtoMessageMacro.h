#pragma once

#ifdef _MSC_VER
    #ifdef _ProtoMessageAPI
        #define ProtoMessageAPI _declspec(dllimport)
    #else
        #define ProtoMessageAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define ProtoMessageAPI __attribute__ ((visibility("default")))
    #else
        #define ProtoMessageAPI
    #endif
#endif

#if defined(STATIC_LIB)
    #undef ProtoMessageAPI
    #define ProtoMessageAPI 
#endif