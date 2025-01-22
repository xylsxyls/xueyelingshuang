#pragma once

#ifdef _MSC_VER
    #ifdef _AssignThreadManagerAPI
        #define AssignThreadManagerAPI _declspec(dllimport)
    #else
        #define AssignThreadManagerAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define AssignThreadManagerAPI __attribute__ ((visibility("default")))
    #else
        #define AssignThreadManagerAPI
    #endif
#endif