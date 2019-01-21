#pragma once

#ifdef _CorrespondParamAPI
#define CorrespondParamAPI _declspec(dllimport)
#else
#define CorrespondParamAPI _declspec(dllexport)
#endif

#if defined(STATIC_LIB)
#undef CorrespondParamAPI
#define CorrespondParamAPI 
#endif