#pragma once

#ifdef _coreAPI
#define coreAPI _declspec(dllimport)
#else
#define coreAPI _declspec(dllexport)
#endif