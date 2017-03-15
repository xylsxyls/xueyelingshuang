#pragma once

#ifdef _CKeyboardAPI
	#define CKeyboardAPI _declspec(dllimport)
#else
	#define CKeyboardAPI _declspec(dllexport)
#endif
