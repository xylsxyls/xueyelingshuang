#pragma once

//#ifdef _JsonCppAPI
//#define JsonCppAPI _declspec(dllimport)
//#else
//#define JsonCppAPI _declspec(dllexport)
//#endif

#ifdef _JsonCppAPI
#define JSON_API _declspec(dllimport)
#else
#define JSON_API _declspec(dllexport)
#endif

#if defined(STATIC_LIB)
#undef JSON_API
#define JSON_API 
#endif