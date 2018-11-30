#pragma once

#ifdef _ProcessClientAPI
#define ProcessClientAPI _declspec(dllimport)
#else
#define ProcessClientAPI _declspec(dllexport)
#endif

#if defined(STATIC_LIB)
#undef ProcessClientAPI
#define ProcessClientAPI 
#endif