#pragma once

#ifdef _MSC_VER
    #ifdef _CompressAPI
        #define CompressAPI _declspec(dllimport)
    #else
        #define CompressAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define CompressAPI __attribute__ ((visibility("default")))
    #else
        #define CompressAPI
    #endif
#endif