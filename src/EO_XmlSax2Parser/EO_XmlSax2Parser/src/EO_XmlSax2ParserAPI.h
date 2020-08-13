#pragma once
#ifdef _WIN32
#define _EO_XmlSax2ParserAPI
#endif
#include "EO_XmlSax2Parser.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"EO_XmlSax2Parserd.lib")
#else
#pragma comment(lib,"EO_XmlSax2Parser.lib")
#endif
#endif