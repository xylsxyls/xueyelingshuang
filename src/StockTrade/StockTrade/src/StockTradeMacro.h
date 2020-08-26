#pragma once

#ifdef _MSC_VER
    #ifdef _StockTradeAPI
        #define StockTradeAPI _declspec(dllimport)
    #else
        #define StockTradeAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define StockTradeAPI __attribute__ ((visibility("default")))
    #else
        #define StockTradeAPI
    #endif
#endif

#if defined(STATIC_LIB)
    #undef StockTradeAPI
    #define StockTradeAPI 
#endif