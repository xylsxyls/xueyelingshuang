#pragma once

#ifdef _CLogAPI
#define CLogAPI _declspec(dllimport)
#else
#define CLogAPI _declspec(dllexport)
#endif