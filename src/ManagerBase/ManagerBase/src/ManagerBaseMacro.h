#pragma once

#ifdef _ManagerBaseAPI
#define ManagerBaseAPI _declspec(dllimport)
#else
#define ManagerBaseAPI _declspec(dllexport)
#endif

#if defined(STATIC_LIB)
#undef ManagerBaseAPI
#define ManagerBaseAPI 
#endif