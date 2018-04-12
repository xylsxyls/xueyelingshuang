#pragma once

#ifdef _ControlsAPI
	#define ControlsAPI _declspec(dllexport)
#else
	#define ControlsAPI _declspec(dllimport)
#endif