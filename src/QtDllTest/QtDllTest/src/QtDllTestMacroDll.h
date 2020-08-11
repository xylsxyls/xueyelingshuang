#pragma once

#ifdef _MSC_VER
    #ifdef _QtDllTestAPI
        #define QtDllTestAPI _declspec(dllimport)
    #else
        #define QtDllTestAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define QtDllTestAPI __attribute__ ((visibility("default")))
    #else
        #define QtDllTestAPI
    #endif
#endif