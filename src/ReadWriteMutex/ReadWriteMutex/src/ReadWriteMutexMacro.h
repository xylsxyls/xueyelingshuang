#pragma once

#ifdef _ReadWriteMutexAPI
#define ReadWriteMutexAPI _declspec(dllimport)
#else
#define ReadWriteMutexAPI _declspec(dllexport)
#endif

#if defined(STATIC_LIB)
#undef ReadWriteMutexAPI
#define ReadWriteMutexAPI 
#endif