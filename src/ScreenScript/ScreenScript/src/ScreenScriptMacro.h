#pragma once

#ifdef _MSC_VER
    #ifdef _ScreenScriptAPI
        #define ScreenScriptAPI _declspec(dllimport)
    #else
        #define ScreenScriptAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define ScreenScriptAPI __attribute__ ((visibility("default")))
    #else
        #define ScreenScriptAPI
    #endif
#endif

#if defined(STATIC_LIB)
    #undef ScreenScriptAPI
    #define ScreenScriptAPI 
#endif