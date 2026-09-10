#pragma once

#ifdef _MSC_VER
    #ifdef _BatchPrinterCoreAPI
        #define BatchPrinterCoreAPI _declspec(dllimport)
    #else
        #define BatchPrinterCoreAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define BatchPrinterCoreAPI __attribute__ ((visibility("default")))
    #else
        #define BatchPrinterCoreAPI
    #endif
#endif

#if defined(STATIC_LIB)
    #undef BatchPrinterCoreAPI
    #define BatchPrinterCoreAPI 
#endif