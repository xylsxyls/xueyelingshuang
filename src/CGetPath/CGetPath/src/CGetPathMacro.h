#pragma once

#ifdef _CGetPathAPI
#define CGetPathAPI _declspec(dllimport)
#else
#define CGetPathAPI _declspec(dllexport)
#endif

#if defined(STATIC_LIB)
#undef CGetPathAPI
#define CGetPathAPI 
#endif