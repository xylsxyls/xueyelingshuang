#pragma once

#ifdef _DialogManagerAPI
#define DialogManagerAPI _declspec(dllimport)
#else
#define DialogManagerAPI _declspec(dllexport)
#endif

#if defined(STATIC_LIB)
#undef DialogManagerAPI
#define DialogManagerAPI 
#endif