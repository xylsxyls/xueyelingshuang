#pragma once

#ifdef _VariantAPI
#define VariantAPI _declspec(dllimport)
#else
#define VariantAPI _declspec(dllexport)
#endif

#if defined(STATIC_LIB)
#undef VariantAPI
#define VariantAPI 
#endif