#pragma once

#ifdef _LogManagerAPI
#define LogManagerAPI _declspec(dllimport)
#else
#define LogManagerAPI _declspec(dllexport)
#endif