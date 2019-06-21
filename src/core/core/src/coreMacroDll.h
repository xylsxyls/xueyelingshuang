#pragma once

#ifdef _coreAPI
#define coreAPI _declspec(dllexport)
#else
#define coreAPI _declspec(dllimport)
#endif