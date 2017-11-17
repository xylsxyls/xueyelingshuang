#pragma once

#ifdef _CHookAPI
#define CHookAPI _declspec(dllimport)
#else
#define CHookAPI _declspec(dllexport)
#endif