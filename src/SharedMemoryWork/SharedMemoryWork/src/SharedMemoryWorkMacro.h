#pragma once

#ifdef _SharedMemoryWorkAPI
#define SharedMemoryWorkAPI _declspec(dllimport)
#else
#define SharedMemoryWorkAPI _declspec(dllexport)
#endif

#if defined(STATIC_LIB)
#undef SharedMemoryWorkAPI
#define SharedMemoryWorkAPI 
#endif