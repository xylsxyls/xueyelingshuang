#pragma once

#ifdef _ConfigManagerAPI
#define ConfigManagerAPI _declspec(dllimport)
#else
#define ConfigManagerAPI _declspec(dllexport)
#endif