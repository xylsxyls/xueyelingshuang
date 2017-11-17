#pragma once

#ifdef _RectAPI
#define RectAPI _declspec(dllimport)
#else
#define RectAPI _declspec(dllexport)
#endif

#if defined(STATIC_LIB)
#undef RectAPI
#define RectAPI 
#endif