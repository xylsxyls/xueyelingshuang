#pragma once

#ifdef _MSC_VER
    #ifdef _SQLiteAPI
        #define SQLiteAPI _declspec(dllimport)
    #else
        #define SQLiteAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define SQLiteAPI __attribute__ ((visibility("default")))
    #else
        #define SQLiteAPI
    #endif
#endif