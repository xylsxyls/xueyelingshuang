#pragma once

#ifdef _CCharsetAPI
#define CCharsetAPI _declspec(dllimport)
#else
#define CCharsetAPI _declspec(dllexport)
#endif

#if defined(STATIC_LIB)
#undef CCharsetAPI
#define CCharsetAPI 
#endif