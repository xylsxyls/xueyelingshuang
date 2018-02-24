#pragma once

#ifdef _ColorAPI
#define ColorAPI _declspec(dllimport)
#else
#define ColorAPI _declspec(dllexport)
#endif

#if defined(STATIC_LIB)
#undef ColorAPI
#define ColorAPI 
#endif