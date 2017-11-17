#pragma once

#ifdef _PointAPI
#define PointAPI _declspec(dllimport)
#else
#define PointAPI _declspec(dllexport)
#endif

#if defined(STATIC_LIB)
#undef PointAPI
#define PointAPI 
#endif