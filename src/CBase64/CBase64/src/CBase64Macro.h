#pragma once

#ifdef _CBase64API
#define CBase64API _declspec(dllimport)
#else
#define CBase64API _declspec(dllexport)
#endif

#if defined(STATIC_LIB)
#undef CBase64API
#define CBase64API 
#endif