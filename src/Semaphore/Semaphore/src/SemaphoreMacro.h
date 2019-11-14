#pragma once

#ifdef _SemaphoreAPI
#define SemaphoreAPI _declspec(dllimport)
#else
#define SemaphoreAPI _declspec(dllexport)
#endif

#if defined(STATIC_LIB)
#undef SemaphoreAPI
#define SemaphoreAPI 
#endif