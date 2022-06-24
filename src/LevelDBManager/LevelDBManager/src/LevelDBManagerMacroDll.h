#pragma once

#ifdef _MSC_VER
    #ifdef _LevelDBManagerAPI
        #define LevelDBManagerAPI _declspec(dllimport)
    #else
        #define LevelDBManagerAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define LevelDBManagerAPI __attribute__ ((visibility("default")))
    #else
        #define LevelDBManagerAPI
    #endif
#endif