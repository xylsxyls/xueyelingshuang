#pragma once

#ifdef _MSC_VER
    #ifdef _StockMarketAPI
        #define StockMarketAPI _declspec(dllimport)
    #else
        #define StockMarketAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define StockMarketAPI __attribute__ ((visibility("default")))
    #else
        #define StockMarketAPI
    #endif
#endif