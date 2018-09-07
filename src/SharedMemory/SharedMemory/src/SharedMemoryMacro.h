#pragma once

#ifdef _SharedMemoryAPI
#define SharedMemoryAPI _declspec(dllimport)
#else
#define SharedMemoryAPI _declspec(dllexport)
#endif

#if defined(STATIC_LIB)
#undef SharedMemoryAPI
#define SharedMemoryAPI 
#endif