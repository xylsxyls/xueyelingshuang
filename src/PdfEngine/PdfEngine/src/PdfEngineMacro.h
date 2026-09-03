#pragma once

#ifdef _MSC_VER
    #ifdef _PdfEngineAPI
        #define PdfEngineAPI _declspec(dllimport)
    #else
        #define PdfEngineAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define PdfEngineAPI __attribute__ ((visibility("default")))
    #else
        #define PdfEngineAPI
    #endif
#endif

#if defined(STATIC_LIB)
    #undef PdfEngineAPI
    #define PdfEngineAPI
#endif