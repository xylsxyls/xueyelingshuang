#pragma once

#ifdef _CSystemAPI
#define CSystemAPI _declspec(dllimport)
#else
#define CSystemAPI _declspec(dllexport)
#endif

#if defined(STATIC_LIB)
#undef CSystemAPI
#define CSystemAPI 
#endif