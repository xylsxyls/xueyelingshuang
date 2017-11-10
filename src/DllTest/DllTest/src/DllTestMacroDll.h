#pragma once

#ifdef _DllTestAPI
#define DllTestAPI _declspec(dllimport)
#else
#define DllTestAPI _declspec(dllexport)
#endif