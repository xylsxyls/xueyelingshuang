#pragma once

#ifdef _MSC_VER
    #ifdef _LumaPlayerCoreAPI
        #define LumaPlayerCoreAPI _declspec(dllimport)
    #else
        #define LumaPlayerCoreAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define LumaPlayerCoreAPI __attribute__ ((visibility("default")))
    #else
        #define LumaPlayerCoreAPI
    #endif
#endif

#if defined(STATIC_LIB)
    #undef LumaPlayerCoreAPI
    #define LumaPlayerCoreAPI 
#endif