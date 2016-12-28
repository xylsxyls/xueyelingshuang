#pragma once

#ifdef _CSystemAPI
	#define CSystemAPI _declspec(dllimport)
#else
	#define CSystemAPI _declspec(dllexport)
#endif
