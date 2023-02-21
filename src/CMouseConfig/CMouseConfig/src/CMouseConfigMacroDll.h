#pragma once

#ifdef _MSC_VER
    #ifdef _CMouseConfigAPI
        #define CMouseConfigAPI _declspec(dllimport)
    #else
        #define CMouseConfigAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define CMouseConfigAPI __attribute__ ((visibility("default")))
    #else
        #define CMouseConfigAPI
    #endif
#endif