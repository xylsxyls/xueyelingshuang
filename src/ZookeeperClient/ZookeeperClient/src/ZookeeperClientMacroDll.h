#pragma once

#ifdef _MSC_VER
    #ifdef _ZookeeperClientAPI
        #define ZookeeperClientAPI _declspec(dllimport)
    #else
        #define ZookeeperClientAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define ZookeeperClientAPI __attribute__ ((visibility("default")))
    #else
        #define ZookeeperClientAPI
    #endif
#endif