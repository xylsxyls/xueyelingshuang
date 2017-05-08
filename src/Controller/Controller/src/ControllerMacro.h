#pragma once

#ifdef _ControllerAPI
	#define ControllerAPI _declspec(dllimport)
#else
	#define ControllerAPI _declspec(dllexport)
#endif
