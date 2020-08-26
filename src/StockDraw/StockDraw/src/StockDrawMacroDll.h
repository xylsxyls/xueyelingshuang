#pragma once

#ifdef _MSC_VER
    #ifdef _StockDrawAPI
        #define StockDrawAPI _declspec(dllimport)
    #else
        #define StockDrawAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define StockDrawAPI __attribute__ ((visibility("default")))
    #else
        #define StockDrawAPI
    #endif
#endif