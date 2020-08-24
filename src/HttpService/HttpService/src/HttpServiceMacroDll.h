#pragma once

#ifdef _MSC_VER
    #ifdef _HttpServiceAPI
        #define HttpServiceAPI _declspec(dllimport)
    #else
        #define HttpServiceAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define HttpServiceAPI __attribute__ ((visibility("default")))
    #else
        #define HttpServiceAPI
    #endif
#endif