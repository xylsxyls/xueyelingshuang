#pragma once

#ifdef _MSC_VER
    #ifdef _JsonCppAPI
        #define JsonCppAPI _declspec(dllimport)
    #else
        #define JsonCppAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define JsonCppAPI __attribute__ ((visibility("default")))
    #else
        #define JsonCppAPI
    #endif
#endif