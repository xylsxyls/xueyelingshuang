#pragma once

#ifdef _CAceHttpServerAPI
	#define CAceHttpServerAPI _declspec(dllimport)
#else
	#define CAceHttpServerAPI _declspec(dllexport)
#endif
