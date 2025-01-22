#pragma once

#ifdef _MSC_VER
    #ifdef _CDumpAPI
        #define CDumpAPI _declspec(dllimport)
    #else
        #define CDumpAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define CDumpAPI __attribute__ ((visibility("default")))
    #else
        #define CDumpAPI
    #endif
#endif