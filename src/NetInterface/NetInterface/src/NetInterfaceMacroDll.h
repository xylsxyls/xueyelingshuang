#pragma once

#ifdef _MSC_VER
    #ifdef _NetInterfaceAPI
        #define NetInterfaceAPI _declspec(dllimport)
    #else
        #define NetInterfaceAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define NetInterfaceAPI __attribute__ ((visibility("default")))
    #else
        #define NetInterfaceAPI
    #endif
#endif