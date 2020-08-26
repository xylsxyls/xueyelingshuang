#pragma once

#ifdef _MSC_VER
    #ifdef _StockMysqlAPI
        #define StockMysqlAPI _declspec(dllimport)
    #else
        #define StockMysqlAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define StockMysqlAPI __attribute__ ((visibility("default")))
    #else
        #define StockMysqlAPI
    #endif
#endif

#if defined(STATIC_LIB)
    #undef StockMysqlAPI
    #define StockMysqlAPI 
#endif