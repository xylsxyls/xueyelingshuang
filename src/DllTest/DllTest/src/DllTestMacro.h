#pragma once

#ifdef _DllTestAPI
#define DllTestAPI _declspec(dllimport)
#else
#define DllTestAPI _declspec(dllexport)
#endif

#if defined(STATIC_LIB)
#undef DllTestAPI
#define DllTestAPI 
#endif