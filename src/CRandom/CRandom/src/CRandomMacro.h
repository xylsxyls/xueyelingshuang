#pragma once

#ifdef _CRandomAPI
#define CRandomAPI _declspec(dllimport)
#else
#define CRandomAPI _declspec(dllexport)
#endif

#if defined(STATIC_LIB)
#undef CRandomAPI
#define CRandomAPI 
#endif