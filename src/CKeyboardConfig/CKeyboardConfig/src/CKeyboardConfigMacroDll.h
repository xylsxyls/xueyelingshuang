#pragma once

#ifdef _MSC_VER
    #ifdef _CKeyboardConfigAPI
        #define CKeyboardConfigAPI _declspec(dllimport)
    #else
        #define CKeyboardConfigAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define CKeyboardConfigAPI __attribute__ ((visibility("default")))
    #else
        #define CKeyboardConfigAPI
    #endif
#endif