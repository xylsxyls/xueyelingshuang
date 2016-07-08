#pragma once

#ifdef _CiniAPI
	#define CiniAPI _declspec(dllimport)
#else
	#define CiniAPI _declspec(dllexport)
#endif
