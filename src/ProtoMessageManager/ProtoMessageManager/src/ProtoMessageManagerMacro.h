#pragma once

#ifdef _MSC_VER
    #ifdef _ProtoMessageManagerAPI
        #define ProtoMessageManagerAPI _declspec(dllimport)
    #else
        #define ProtoMessageManagerAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define ProtoMessageManagerAPI __attribute__ ((visibility("default")))
    #else
        #define ProtoMessageManagerAPI
    #endif
#endif

#if defined(STATIC_LIB)
    #undef ProtoMessageManagerAPI
    #define ProtoMessageManagerAPI 
#endif