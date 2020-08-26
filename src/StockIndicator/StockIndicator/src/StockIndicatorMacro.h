#pragma once

#ifdef _MSC_VER
    #ifdef _StockIndicatorAPI
        #define StockIndicatorAPI _declspec(dllimport)
    #else
        #define StockIndicatorAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define StockIndicatorAPI __attribute__ ((visibility("default")))
    #else
        #define StockIndicatorAPI
    #endif
#endif

#if defined(STATIC_LIB)
    #undef StockIndicatorAPI
    #define StockIndicatorAPI 
#endif