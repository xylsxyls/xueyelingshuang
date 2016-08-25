#pragma once

#ifdef _CBase64API
	#define CBase64API _declspec(dllimport)
#else
	#define CBase64API _declspec(dllexport)
#endif
