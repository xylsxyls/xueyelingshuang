#pragma once

#ifdef _MSC_VER
    #ifdef _StockStrategyAPI
        #define StockStrategyAPI _declspec(dllimport)
    #else
        #define StockStrategyAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define StockStrategyAPI __attribute__ ((visibility("default")))
    #else
        #define StockStrategyAPI
    #endif
#endif

#if defined(STATIC_LIB)
    #undef StockStrategyAPI
    #define StockStrategyAPI 
#endif