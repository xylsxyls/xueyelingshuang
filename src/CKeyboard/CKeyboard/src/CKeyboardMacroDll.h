#pragma once

#ifdef _MSC_VER
    #ifdef _CKeyboardAPI
        #define CKeyboardAPI _declspec(dllimport)
    #else
        #define CKeyboardAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define CKeyboardAPI __attribute__ ((visibility("default")))
    #else
        #define CKeyboardAPI
    #endif
#endif