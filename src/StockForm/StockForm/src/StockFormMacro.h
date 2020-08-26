#pragma once

#ifdef _MSC_VER
    #ifdef _StockFormAPI
        #define StockFormAPI _declspec(dllimport)
    #else
        #define StockFormAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define StockFormAPI __attribute__ ((visibility("default")))
    #else
        #define StockFormAPI
    #endif
#endif

#if defined(STATIC_LIB)
    #undef StockFormAPI
    #define StockFormAPI 
#endif