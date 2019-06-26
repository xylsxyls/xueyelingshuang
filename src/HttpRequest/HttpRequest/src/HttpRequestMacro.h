#pragma once

#ifdef _HttpRequestAPI
#define HttpRequestAPI _declspec(dllimport)
#else
#define HttpRequestAPI _declspec(dllexport)
#endif

#if defined(STATIC_LIB)
#undef HttpRequestAPI
#define HttpRequestAPI 
#endif