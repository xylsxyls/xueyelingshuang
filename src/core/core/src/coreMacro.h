#pragma once

#ifdef _coreAPI
#define coreAPI _declspec(dllimport)
#else
#define coreAPI _declspec(dllexport)
#endif

#if defined(STATIC_LIB)
#undef coreAPI
#define coreAPI 
#endif