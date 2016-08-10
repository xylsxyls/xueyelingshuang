#pragma once

#ifdef _CAceTCPServerAPI
	#define CAceTCPServerAPI _declspec(dllimport)
#else
	#define CAceTCPServerAPI _declspec(dllexport)
#endif
