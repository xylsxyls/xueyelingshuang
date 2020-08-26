#pragma once

#ifdef _MSC_VER
    #ifdef _StockStorageAPI
        #define StockStorageAPI _declspec(dllimport)
    #else
        #define StockStorageAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define StockStorageAPI __attribute__ ((visibility("default")))
    #else
        #define StockStorageAPI
    #endif
#endif

#if defined(STATIC_LIB)
    #undef StockStorageAPI
    #define StockStorageAPI 
#endif