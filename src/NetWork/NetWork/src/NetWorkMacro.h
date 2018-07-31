#pragma once

#ifdef _NetWorkAPI
#define NetWorkAPI _declspec(dllimport)
#else
#define NetWorkAPI _declspec(dllexport)
#endif

#if defined(STATIC_LIB)
#undef NetWorkAPI
#define NetWorkAPI 
#endif