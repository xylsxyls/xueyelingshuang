#pragma once

#ifdef _MSC_VER
    #ifdef _StockFundAPI
        #define StockFundAPI _declspec(dllimport)
    #else
        #define StockFundAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define StockFundAPI __attribute__ ((visibility("default")))
    #else
        #define StockFundAPI
    #endif
#endif

#if defined(STATIC_LIB)
    #undef StockFundAPI
    #define StockFundAPI 
#endif