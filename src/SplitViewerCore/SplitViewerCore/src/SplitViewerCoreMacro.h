#pragma once

#ifdef _MSC_VER
    #ifdef _SplitViewerCoreAPI
        #define SplitViewerCoreAPI _declspec(dllimport)
    #else
        #define SplitViewerCoreAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define SplitViewerCoreAPI __attribute__ ((visibility("default")))
    #else
        #define SplitViewerCoreAPI
    #endif
#endif

#if defined(STATIC_LIB)
    #undef SplitViewerCoreAPI
    #define SplitViewerCoreAPI 
#endif