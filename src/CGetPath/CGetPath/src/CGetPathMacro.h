#pragma once

#ifdef _CGetPathAPI
	#define CGetPathAPI _declspec(dllimport)
#else
	#define CGetPathAPI _declspec(dllexport)
#endif
