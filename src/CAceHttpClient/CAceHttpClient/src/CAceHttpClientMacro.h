#pragma once

#ifdef _CAceHttpClientAPI
	#define CAceHttpClientAPI _declspec(dllimport)
#else
	#define CAceHttpClientAPI _declspec(dllexport)
#endif
