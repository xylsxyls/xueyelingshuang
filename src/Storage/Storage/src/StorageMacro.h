#pragma once

#ifdef _StorageAPI
#define StorageAPI _declspec(dllimport)
#else
#define StorageAPI _declspec(dllexport)
#endif

#if defined(STATIC_LIB)
#undef StorageAPI
#define StorageAPI 
#endif