#pragma once

#ifdef _DialogManagerAPI
#define DialogManagerAPI _declspec(dllexport)
#else
#define DialogManagerAPI _declspec(dllimport)
#endif

#if defined(STATIC_LIB)
#undef DialogManagerAPI
#define DialogManagerAPI 
#endif