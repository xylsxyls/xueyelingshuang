#pragma once

#ifdef _VariantAPI
#define VariantAPI _declspec(dllimport)
#else
#define VariantAPI _declspec(dllexport)
#endif