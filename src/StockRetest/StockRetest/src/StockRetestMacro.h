#pragma once

#ifdef _MSC_VER
    #ifdef _StockRetestAPI
        #define StockRetestAPI _declspec(dllimport)
    #else
        #define StockRetestAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define StockRetestAPI __attribute__ ((visibility("default")))
    #else
        #define StockRetestAPI
    #endif
#endif

#if defined(STATIC_LIB)
    #undef StockRetestAPI
    #define StockRetestAPI 
#endif