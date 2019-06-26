#pragma once

#ifdef _EO_XmlSax2ParserAPI
#define EO_XmlSax2ParserAPI _declspec(dllimport)
#else
#define EO_XmlSax2ParserAPI _declspec(dllexport)
#endif

#if defined(STATIC_LIB)
#undef EO_XmlSax2ParserAPI
#define EO_XmlSax2ParserAPI 
#endif