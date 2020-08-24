#pragma once

#ifdef _MSC_VER
    #ifdef _JsonCppAPI
        #define JSON_API _declspec(dllimport)
    #else
        #define JSON_API _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define JSON_API __attribute__ ((visibility("default")))
    #else
        #define JSON_API
    #endif
#endif

#if defined(STATIC_LIB)
    #undef JSON_API
    #define JSON_API 
#endif