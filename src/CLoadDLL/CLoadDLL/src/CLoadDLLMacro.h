#pragma once

#ifdef _CLoadDLLAPI
#define CLoadDLLAPI _declspec(dllimport)
#else
#define CLoadDLLAPI _declspec(dllexport)
#endif

#if defined(STATIC_LIB)
#undef CLoadDLLAPI
#define CLoadDLLAPI 
#endif