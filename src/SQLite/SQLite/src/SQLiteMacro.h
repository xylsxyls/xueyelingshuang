#pragma once

#ifdef _SQLiteAPI
#define SQLiteAPI _declspec(dllimport)
#else
#define SQLiteAPI _declspec(dllexport)
#endif

#if defined(STATIC_LIB)
#undef SQLiteAPI
#define SQLiteAPI 
#endif