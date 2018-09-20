#pragma once

#ifdef _SqlStringAPI
#define SqlStringAPI _declspec(dllimport)
#else
#define SqlStringAPI _declspec(dllexport)
#endif