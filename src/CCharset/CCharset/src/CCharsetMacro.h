#pragma once

#ifdef _CCharsetAPI
	#define CCharsetAPI _declspec(dllimport)
#else
	#define CCharsetAPI _declspec(dllexport)
#endif
