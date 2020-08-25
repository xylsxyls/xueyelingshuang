#pragma once

#ifdef _MSC_VER
    #ifdef _LibuvTcpAPI
        #define LibuvTcpAPI _declspec(dllimport)
    #else
        #define LibuvTcpAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define LibuvTcpAPI __attribute__ ((visibility("default")))
    #else
        #define LibuvTcpAPI
    #endif
#endif