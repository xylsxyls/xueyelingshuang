#pragma once

#ifdef _MSC_VER
    #ifdef _StockSolutionAPI
        #define StockSolutionAPI _declspec(dllimport)
    #else
        #define StockSolutionAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define StockSolutionAPI __attribute__ ((visibility("default")))
    #else
        #define StockSolutionAPI
    #endif
#endif