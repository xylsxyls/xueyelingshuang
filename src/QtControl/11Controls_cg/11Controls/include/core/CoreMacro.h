#pragma once

#ifdef _CoreAPI
	#define CoreAPI _declspec(dllimport)
#else
	#define CoreAPI _declspec(dllexport)
#endif