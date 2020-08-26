#pragma once

#ifdef _MSC_VER
    #ifdef _LogSenderAPI
        #define LogSenderAPI _declspec(dllimport)
    #else
        #define LogSenderAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define LogSenderAPI __attribute__ ((visibility("default")))
    #else
        #define LogSenderAPI
    #endif
#endif