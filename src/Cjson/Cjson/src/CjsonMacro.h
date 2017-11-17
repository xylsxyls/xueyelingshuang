#pragma once

#ifdef _CjsonAPI
#define CjsonAPI _declspec(dllimport)
#else
#define CjsonAPI _declspec(dllexport)
#endif

#if defined(STATIC_LIB)
#undef CjsonAPI
#define CjsonAPI 
#endif