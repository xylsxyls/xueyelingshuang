#pragma once

#ifdef _FastNumberAPI
#define FastNumberAPI _declspec(dllimport)
#else
#define FastNumberAPI _declspec(dllexport)
#endif

#if defined(STATIC_LIB)
#undef FastNumberAPI
#define FastNumberAPI 
#endif