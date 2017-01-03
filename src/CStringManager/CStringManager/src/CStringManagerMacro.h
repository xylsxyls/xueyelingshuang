#pragma once

#ifdef _CStringManagerAPI
	#define CStringManagerAPI _declspec(dllimport)
#else
	#define CStringManagerAPI _declspec(dllexport)
#endif

