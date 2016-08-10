#pragma once

#ifdef _CAceTCPClientAPI
	#define CAceTCPClientAPI _declspec(dllimport)
#else
	#define CAceTCPClientAPI _declspec(dllexport)
#endif
