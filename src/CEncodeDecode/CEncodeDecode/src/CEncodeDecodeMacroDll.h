#pragma once

#ifdef _MSC_VER
    #ifdef _CEncodeDecodeAPI
        #define CEncodeDecodeAPI _declspec(dllimport)
    #else
        #define CEncodeDecodeAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define CEncodeDecodeAPI __attribute__ ((visibility("default")))
    #else
        #define CEncodeDecodeAPI
    #endif
#endif