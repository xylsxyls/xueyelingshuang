#pragma once

#ifdef _MSC_VER
    #ifdef _WidgetFactoryAPI
        #define WidgetFactoryAPI _declspec(dllimport)
    #else
        #define WidgetFactoryAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define WidgetFactoryAPI __attribute__ ((visibility("default")))
    #else
        #define WidgetFactoryAPI
    #endif
#endif

#if defined(STATIC_LIB)
    #undef WidgetFactoryAPI
    #define WidgetFactoryAPI 
#endif