#pragma once

#ifdef _SQLiteAPI
#define SQLiteAPI _declspec(dllimport)
#else
#define SQLiteAPI _declspec(dllexport)
#endif