#pragma once

#ifdef _CLoadDLLAPI
#define CLoadDLLAPI _declspec(dllimport)
#else
#define CLoadDLLAPI _declspec(dllexport)
#endif
