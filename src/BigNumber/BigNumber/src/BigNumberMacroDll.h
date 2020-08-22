#pragma once

#ifdef _MSC_VER
    #ifdef _BigNumberAPI
        #define BigNumberAPI _declspec(dllimport)
    #else
        #define BigNumberAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define BigNumberAPI __attribute__ ((visibility("default")))
    #else
        #define BigNumberAPI
    #endif
#endif