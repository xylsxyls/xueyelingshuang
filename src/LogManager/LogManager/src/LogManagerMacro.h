#pragma once

#ifdef _LogManagerAPI
#define LogManagerAPI _declspec(dllimport)
#else
#define LogManagerAPI _declspec(dllexport)
#endif

#if defined(STATIC_LIB)
#undef LogManagerAPI
#define LogManagerAPI 
#endif