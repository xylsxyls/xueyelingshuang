#pragma once

#ifdef _CTaskAPI
	#define CTaskAPI _declspec(dllimport)
#else
	#define CTaskAPI _declspec(dllexport)
#endif
