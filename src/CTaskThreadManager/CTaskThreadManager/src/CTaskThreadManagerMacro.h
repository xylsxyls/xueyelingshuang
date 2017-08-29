#pragma once

#ifdef _CTaskThreadManagerAPI
	#define CTaskThreadManagerAPI _declspec(dllimport)
#else
	#define CTaskThreadManagerAPI _declspec(dllexport)
#endif
