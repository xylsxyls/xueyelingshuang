#pragma once

#ifdef _MSC_VER
    #ifdef _StockTypeAPI
        #define StockTypeAPI _declspec(dllimport)
    #else
        #define StockTypeAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define StockTypeAPI __attribute__ ((visibility("default")))
    #else
        #define StockTypeAPI
    #endif
#endif