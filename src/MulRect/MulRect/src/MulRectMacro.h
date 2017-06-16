#pragma once

#ifdef _MulRectAPI
	#define MulRectAPI _declspec(dllimport)
#else
	#define MulRectAPI _declspec(dllexport)
#endif
