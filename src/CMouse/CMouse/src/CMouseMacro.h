#pragma once

#ifdef _CMouseAPI
	#define CMouseAPI _declspec(dllimport)
#else
	#define CMouseAPI _declspec(dllexport)
#endif
