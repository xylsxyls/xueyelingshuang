#pragma once

#ifdef _MSC_VER
    #ifdef _CBase64API
        #define CBase64API _declspec(dllimport)
    #else
        #define CBase64API _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define CBase64API __attribute__ ((visibility("default")))
    #else
        #define CBase64API
    #endif
#endif