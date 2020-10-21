#pragma once

#ifdef _MSC_VER
    #ifdef _MsgLinuxAPI
        #define MsgLinuxAPI _declspec(dllimport)
    #else
        #define MsgLinuxAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define MsgLinuxAPI __attribute__ ((visibility("default")))
    #else
        #define MsgLinuxAPI
    #endif
#endif

#if defined(STATIC_LIB)
    #undef MsgLinuxAPI
    #define MsgLinuxAPI 
#endif