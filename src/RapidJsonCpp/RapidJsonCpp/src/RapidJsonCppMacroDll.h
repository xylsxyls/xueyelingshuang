#pragma once

#ifdef _MSC_VER
    #ifdef _RapidJsonCppAPI
        #define RapidJsonCppAPI _declspec(dllimport)
    #else
        #define RapidJsonCppAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define RapidJsonCppAPI __attribute__ ((visibility("default")))
    #else
        #define RapidJsonCppAPI
    #endif
#endif