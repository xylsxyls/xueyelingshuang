#pragma once

#ifdef _MSC_VER
    #define QtControlsAPI _declspec(dllexport)
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define QtControlsAPI __attribute__ ((visibility("default")))
    #else
        #define QtControlsAPI
    #endif
#endif

#if defined(STATIC_LIB)
    #undef QtControlsAPI
    #define QtControlsAPI 
#endif