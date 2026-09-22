#pragma once

#ifdef _MSC_VER
    #ifdef _PdfReaderCoreAPI
        #define PdfReaderCoreAPI _declspec(dllimport)
    #else
        #define PdfReaderCoreAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define PdfReaderCoreAPI __attribute__ ((visibility("default")))
    #else
        #define PdfReaderCoreAPI
    #endif
#endif