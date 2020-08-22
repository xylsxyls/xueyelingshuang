#pragma once

#ifdef _MSC_VER
    #ifdef _CorrespondParamAPI
        #define CorrespondParamAPI _declspec(dllimport)
    #else
        #define CorrespondParamAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define CorrespondParamAPI __attribute__ ((visibility("default")))
    #else
        #define CorrespondParamAPI
    #endif
#endif