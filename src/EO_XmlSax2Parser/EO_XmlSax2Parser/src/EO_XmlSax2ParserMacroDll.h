#pragma once

#ifdef _MSC_VER
    #ifdef _EO_XmlSax2ParserAPI
        #define EO_XmlSax2ParserAPI _declspec(dllimport)
    #else
        #define EO_XmlSax2ParserAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define EO_XmlSax2ParserAPI __attribute__ ((visibility("default")))
    #else
        #define EO_XmlSax2ParserAPI
    #endif
#endif