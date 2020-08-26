#pragma once

#ifdef _MSC_VER
    #ifdef _StockChargeAPI
        #define StockChargeAPI _declspec(dllimport)
    #else
        #define StockChargeAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define StockChargeAPI __attribute__ ((visibility("default")))
    #else
        #define StockChargeAPI
    #endif
#endif

#if defined(STATIC_LIB)
    #undef StockChargeAPI
    #define StockChargeAPI 
#endif